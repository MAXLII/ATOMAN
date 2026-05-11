#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

// 包含配置文件
#include "fw_info.h"



// CRC32多项式
#define CRC32_POLY 0xEDB88320

/**
 * @brief 计算CRC32校验值（无查表法）
 * @param data 输入数据指针
 * @param length 数据长度
 * @return CRC32校验值
 */
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLY;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief 计算footer结构的CRC32（不包括crc32字段本身）
 * @param footer 指向footer结构的指针
 * @return 计算出的CRC32值
 */
uint32_t calculate_footer_crc(const footer_t *footer) {
    // 计算除crc32字段之外的所有字段
    size_t data_length = sizeof(footer_t) - sizeof(footer->crc32);
    return calculate_crc32((const uint8_t*)footer, data_length);
}

/**
 * @brief 获取文件大小
 * @param filename 文件名
 * @return 文件大小（字节），失败返回0
 */
uint32_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return (uint32_t)st.st_size;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("用法: %s input.bin [output.bin] [commit_id]\n", argv[0]);
        printf("示例: %s firmware.bin\n", argv[0]);
        printf("示例: %s firmware.bin output.bin abc123f\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = (argc > 2) ? argv[2] : "firmware_with_footer.bin";
    
    // 获取原始文件大小
    uint32_t original_file_size = get_file_size(input_file);
    if (original_file_size == 0) {
        printf("错误: 无法获取文件大小或文件为空: %s\n", input_file);
        return 1;
    }
    
    // 自动组合版本信息
    uint32_t version = COMPOSE_VERSION(HARD_VER, DEVICE_VENDOR, RELEASE_VER, DEBUG_VER);
    
    // 获取commit ID
    char commit_id[17] = {0};
    if (argc > 3) {
        // 使用传入的commit ID
        strncpy(commit_id, argv[3], sizeof(commit_id)-1);
    } else {
        // 尝试自动获取Git commit ID (16位)
        FILE *git = popen("git rev-parse --short=16 HEAD 2>nul", "r");
        if (git && fgets(commit_id, sizeof(commit_id)-1, git)) {
            // 移除换行符
            commit_id[strcspn(commit_id, "\r\n")] = 0;
            pclose(git);
        } else {
            // 回退到获取完整commit ID
            FILE *git_full = popen("git rev-parse HEAD 2>nul", "r");
            if (git_full) {
                char full_commit[41] = {0};
                if (fgets(full_commit, sizeof(full_commit)-1, git_full)) {
                    full_commit[strcspn(full_commit, "\r\n")] = 0;
                    // 取前16位
                    strncpy(commit_id, full_commit, 16);
                    commit_id[16] = '\0';
                }
                pclose(git_full);
            } else {
                strcpy(commit_id, "NOGIT");
            }
        }
    }
    
    // 准备footer数据
    footer_t footer = {
        .unix_time = (uint32_t)time(NULL),
        .fw_type = FW_TYPE,
        .version = version,
        .file_size = original_file_size,  // 设置原始文件大小
        .module_id = MODULE_HOST
    };
    
    memset(footer.commit_id, 0, sizeof(footer.commit_id));
    strncpy((char*)footer.commit_id, commit_id, sizeof(footer.commit_id)-1);
    
    // 计算CRC32（不包括crc32字段本身）
    footer.crc32 = calculate_footer_crc(&footer);
    
    // 打开输入文件
    FILE *fin = fopen(input_file, "rb");
    if (!fin) {
        printf("错误: 无法打开输入文件 %s\n", input_file);
        return 1;
    }
    
    // 创建输出文件
    FILE *fout = fopen(output_file, "wb");
    if (!fout) {
        printf("错误: 无法创建输出文件 %s\n", output_file);
        fclose(fin);
        return 1;
    }
    
    // 复制原文件内容
    int ch;
    uint32_t bytes_copied = 0;
    while ((ch = fgetc(fin)) != EOF) {
        fputc(ch, fout);
        bytes_copied++;
    }
    
    // 验证复制的字节数
    if (bytes_copied != original_file_size) {
        printf("警告: 文件复制不完整，期望 %u 字节，实际 %u 字节\n", 
               original_file_size, bytes_copied);
    }
    
    // 添加footer
    size_t footer_written = fwrite(&footer, sizeof(footer), 1, fout);
    if (footer_written != 1) {
        printf("错误: 写入footer失败\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }
    
    fclose(fin);
    fclose(fout);
    
    // 获取最终文件大小
    uint32_t final_file_size = get_file_size(output_file);
    
    // 解析版本信息用于显示
    uint8_t hard_ver = (version >> 24) & 0xFF;
    uint8_t vendor = (version >> 16) & 0xFF;
    uint8_t release_ver = (version >> 8) & 0xFF;
    uint8_t debug_ver = version & 0xFF;
    
    printf("成功添加文件尾信息:\n");
    printf("  输入文件: %s\n", input_file);
    printf("  输出文件: %s\n", output_file);
    printf("  原始大小: %u 字节\n", original_file_size);
    printf("  最终大小: %u 字节\n", final_file_size);
    printf("  Footer大小: %zu 字节\n", sizeof(footer));
    printf("  Unix时间: %u\n", footer.unix_time);
    printf("  版本: 0x%08X\n", version);
    printf("    硬件版本: %d\n", hard_ver);
    printf("    设备商: %d\n", vendor);
    printf("    发布版本: %d\n", release_ver);
    printf("    调试版本: %d\n", debug_ver);
    printf("  模块主机: 0x%02X\n", footer.module_id);
    printf("  固件类型: %s", footer.fw_type == FW_TYPE_IAP ? "IAP" : "ISP");
    printf("  Commit: %s\n", commit_id);
    printf("  CRC32: 0x%08X\n", footer.crc32);
    printf("  总大小增加: %u 字节\n", final_file_size - original_file_size);
    
    return 0;
}
