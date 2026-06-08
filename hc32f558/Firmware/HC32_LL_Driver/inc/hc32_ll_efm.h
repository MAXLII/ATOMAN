/**
 *******************************************************************************
 * @file  hc32_ll_efm.h
 * @brief This file contains all the functions prototypes of the EFM driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
   2026-02-02       CDT             Rename EFM_ECC_EXP_TYPE_RESET to EFM_ECC_EXP_TYPE_RST
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */
#ifndef __HC32_LL_EFM_H__
#define __HC32_LL_EFM_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"
/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_EFM
 * @{
 */

#if (LL_EFM_ENABLE == DDL_ON)

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EFM_Pre-processor_Macros EFM Pre-processor Macros
 * @{
 */

#ifndef __EFM_FUNC
#define __EFM_FUNC                      __RAM_FUNC
#endif

/**
 * @}
 */
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup EFM_Global_Types EFM Global Types
 * @{
 */

/**
 * @brief EFM unique ID definition
 */
typedef struct {
    uint32_t            u32UniqueID0;      /*!< unique ID 0.       */
    uint32_t            u32UniqueID1;      /*!< unique ID 1.       */
    uint32_t            u32UniqueID2;      /*!< unique ID 2.       */
} stc_efm_unique_id_t;

typedef struct {
    uint32_t u32State;
    uint32_t u32Addr;
    uint32_t u32Size;
} stc_efm_remap_init_t;

/**
 * @brief EFM location definition
 */
typedef struct {
    uint8_t             u8X_Location;      /*!< X location.       */
    uint8_t             u8Y_Location;      /*!< Y location.       */
} stc_efm_location_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EFM_Global_Macros EFM Global Macros
 * @{
 */
/**
 * @defgroup EFM_Address EFM Address Area
 * @{
 */
#define EFM_START_ADDR                  (0x00000000UL)    /*!< Flash start address */
#define EFM_END_ADDR                    (0x0007FFFFUL)    /*!< Flash end address */
#define EFM_FLASH_1_START_ADDR          (0x00040000UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Chip_Sel EFM Chip Selection
 * @{
 */
#define EFM_CHIP0                       (EFM_FSTP_F0STP)
#define EFM_CHIP1                       (EFM_FSTP_F1STP)
#define EFM_CHIP_ALL                    (EFM_CHIP0 | EFM_CHIP1)

/**
 * @}
 */

/**
 * @defgroup EFM_Bus_Status EFM Bus Status
 * @{
 */
#define EFM_BUS_HOLD                    (0x0UL)     /*!< Bus busy while flash program or erase */
#define EFM_BUS_RELEASE                 (0x1UL)     /*!< Bus release while flash program or erase */
/**
 * @}
 */

/**
 * @defgroup EFM_Wait_Cycle EFM Wait Cycle
 * @{
 */

#define EFM_WAIT_CYCLE0                 (0U)                /*!< Don't insert read wait cycle */
#define EFM_WAIT_CYCLE1                 (1U)                /*!< Insert 1 read wait cycle     */
#define EFM_WAIT_CYCLE2                 (2U)                /*!< Insert 2 read wait cycles    */
#define EFM_WAIT_CYCLE3                 (3U)                /*!< Insert 3 read wait cycles    */
#define EFM_WAIT_CYCLE4                 (4U)                /*!< Insert 4 read wait cycles    */
#define EFM_WAIT_CYCLE5                 (5U)                /*!< Insert 5 read wait cycles    */
#define EFM_WAIT_CYCLE6                 (6U)                /*!< Insert 6 read wait cycles    */
#define EFM_WAIT_CYCLE7                 (7U)                /*!< Insert 7 read wait cycles    */
#define EFM_WAIT_CYCLE8                 (8U)                /*!< Insert 8 read wait cycles    */
#define EFM_WAIT_CYCLE9                 (9U)                /*!< Insert 9 read wait cycles    */
#define EFM_WAIT_CYCLE10                (10U)               /*!< Insert 10 read wait cycles   */
#define EFM_WAIT_CYCLE11                (11U)               /*!< Insert 11 read wait cycles   */
#define EFM_WAIT_CYCLE12                (12U)               /*!< Insert 12 read wait cycles   */
#define EFM_WAIT_CYCLE13                (13U)               /*!< Insert 13 read wait cycles   */
#define EFM_WAIT_CYCLE14                (14U)               /*!< Insert 14 read wait cycles   */
#define EFM_WAIT_CYCLE15                (15U)               /*!< Insert 15 read wait cycles   */
#define EFM_WAIT_CYCLE_MAX              (EFM_WAIT_CYCLE15)
/**
 * @}
 */

/**
 * @defgroup EFM_Cache_Select  EFM Cache Select
 * @{
 */
#define EFM_RD_ICACHE                   (EFM_FRMC_ICACHE)   /*!< ICACHE */
#define EFM_RD_DCACHE                   (EFM_FRMC_DCACHE)   /*!< DCACHE */
#define EFM_RD_PREFETCH                 (EFM_FRMC_PREFETE)  /*!< PREFETCH */
#define EFM_CACHE_ALL                   (EFM_RD_ICACHE | EFM_RD_DCACHE | EFM_RD_PREFETCH)

/**
 * @}
 */

/**
 * @defgroup EFM_Swap_Address EFM Swap Address
 * @{
 */
#define EFM_SWAP_ADDR                   (0x03002000UL)
#define EFM_SWAP_DATA                   (0x005A5A5AUL)
/**
 * @}
 */

/**
 * @defgroup EFM_Swap_Immediately EFM Swap Immediately
 * @{
 */
#define EFM_SWAP_IMMED_ENABLE           (0x77U)
#define EFM_SWAP_IMMED_DISABLE          (0x76U)
/**
 * @}
 */

/**
 * @defgroup EFM_WriteLock_Sel EFM Write Protect Lock Selection
 * @{
 */
#define EFM_WRLOCK0                     (EFM_WLOCK_WLOCK_0)     /*!< F0NWPRT0 controlled sector lock   */
#define EFM_WRLOCK4                     (EFM_WLOCK_WLOCK_4)     /*!< F1NWPRT0 controlled sector lock   */
/**
 * @}
 */

/**
 * @defgroup EFM_OperateMode_Sel EFM Operate Mode Selection
 * @{
 */
#define EFM_MD_READONLY                 (0x0UL << EFM_FWMC_PEMOD_POS)   /*!< Read only mode               */
#define EFM_MD_PE_STOP                  (0x1UL << EFM_FWMC_PEMOD_POS)   /*!< Stop program or erase          */
#define EFM_MD_ERASE_ONE_CHIP           (0x2UL << EFM_FWMC_PEMOD_POS)   /*!< A flash Chip erase mode        */
#define EFM_MD_ERASE_SMART              (0x3UL << EFM_FWMC_PEMOD_POS)   /*!< Smart erase mode               */
#define EFM_MD_PGM_SMART1               (0x4UL << EFM_FWMC_PEMOD_POS)   /*!< Smart program mode 1           */
#define EFM_MD_PGM_SMART2               (0x6UL << EFM_FWMC_PEMOD_POS)   /*!< Smart program mode 2           */
#define EFM_MD_ERASE_ALL_CHIP           (0xAUL << EFM_FWMC_PEMOD_POS)   /*!< All chip erase mode            */
#define EFM_MD_PGM_SEQ_SMART1           (0xCUL << EFM_FWMC_PEMOD_POS)   /*!< Smart sequence program mode 1  */
#define EFM_MD_PGM_SEQ_SMART2           (0xEUL << EFM_FWMC_PEMOD_POS)   /*!< Smart sequence program mode 2  */
#define EFM_MD_PGM_SINGLE               (EFM_MD_PGM_SMART2)             /*!< single program mode use smart mode 1 by default   */
#define EFM_MD_ERASE_SECTOR             (EFM_MD_ERASE_SMART)            /*!< sector erase mode use smart erase by default   */
#define EFM_MD_PGM_SEQ                  (EFM_MD_PGM_SEQ_SMART1)         /*!< sequence program mode use smart sequence program mode 1 by default   */
/**
 * @}
 */

/**
 * @defgroup EFM_PGM_UNIT_BYTES_Definition EFM PGM Unit Bytes definition
 * @{
 */
#define EFM_PGM_UNIT_BYTES              (16UL)

/**
 * @}
 */

/**
 * @defgroup EFM_Program_Unit_define EFM Program Unit define
 * @{
 */
#define EFM_PGM_UNIT_WORDS              ((EFM_PGM_UNIT_BYTES + 3U) / 4U)
#ifndef EFM_PGM_PAD_BYTE
#define EFM_PGM_PAD_BYTE                (0xFFU)
#endif
/**
 * @}
 */

/**
 * @defgroup EFM_Flag_Sel  EFM Flag Selection
 * @{
 */
#define EFM_FLAG_ROWERR                 (EFM_FSR_ROWERR0)       /*!< EFM Flash0 sequence program row error flag.        */
#define EFM_FLAG_SWERR                  (EFM_FSR_SWERR0)        /*!< EFM Flash0 smart program/erase error flag.         */
#define EFM_FLAG_RDY                    (EFM_FSR_RDY0)          /*!< EFM Flash0 ready flag.                             */
#define EFM_FLAG_BLKERR                 (EFM_FSR_BRMER0)        /*!< EFM Flash0 blank check error flag                  */
#define EFM_FLAG_ECC_OVF0               (EFM_FSR_ECEROF0)       /*!< EFM Flash0 ECC error record overflow flag.         */
#define EFM_FLAG_COLERR                 (EFM_FSR_COLERR0)       /*!< EFM Flash0 read collide error flag.                */
#define EFM_FLAG_OPTEND                 (EFM_FSR_OPTEND0)       /*!< EFM Flash0 end of operation flag.                  */
#define EFM_FLAG_SPGMEND                (EFM_FSR_SPSEND0)       /*!< EFM Flash0 sequence program date write flag.       */
#define EFM_FLAG_PGSZERR                (EFM_FSR_PGSZERR0)      /*!< EFM Flash0 programming size error flag.            */
#define EFM_FLAG_PEPRTERR               (EFM_FSR_PRTWERR0)      /*!< EFM Flash0 write protect address error flag.       */
#define EFM_FLAG_OTPWERR                (EFM_FSR_OTPWERR0)      /*!< EFM Flash0 otp program/erase error flag.           */
#define EFM_FLAG_ROWERR1                (EFM_FSR_ROWERR1)       /*!< EFM Flash0 sequence program row error flag.        */
#define EFM_FLAG_SWERR1                 (EFM_FSR_SWERR1)        /*!< EFM Flash0 smart program/erase error flag.         */
#define EFM_FLAG_RDY1                   (EFM_FSR_RDY1)          /*!< EFM Flash1 ready flag.                             */
#define EFM_FLAG_BLKERR1                (EFM_FSR_BRMER1)        /*!< EFM Flash1 blank check error flag                  */
#define EFM_FLAG_ECC_OVF1               (EFM_FSR_ECEROF1)       /*!< EFM Flash1 ECC error record overflow flag.         */
#define EFM_FLAG_COLERR1                (EFM_FSR_COLERR1)       /*!< EFM Flash1 read collide error flag.                */
#define EFM_FLAG_OPTEND1                (EFM_FSR_OPTEND1)       /*!< EFM Flash1 end of operation flag.                  */
#define EFM_FLAG_SPGMEND1               (EFM_FSR_SPSEND1)       /*!< EFM Flash1 sequence program date write flag.       */
#define EFM_FLAG_PGSZERR1               (EFM_FSR_PGSZERR1)      /*!< EFM Flash1 programming size error flag.            */
#define EFM_FLAG_PEPRTERR1              (EFM_FSR_PRTWERR1)      /*!< EFM Flash1 write protect address error flag.       */
#define EFM_FLAG_OTPWERR1               (EFM_FSR_OTPWERR1)      /*!< EFM Flash1 otp program/erase error flag.           */
#define EFM_FLAG_BLKERR_ALL             (EFM_FLAG_BLKERR | EFM_FLAG_BLKERR1)

#define EFM_FLAG_RDY_ALL                (EFM_FLAG_RDY | EFM_FLAG_RDY1)

#define EFM_FLAG_ERR                    (EFM_FLAG_ROWERR | EFM_FLAG_SWERR | EFM_FLAG_BLKERR | EFM_FLAG_COLERR | \
                                         EFM_FLAG_SPGMEND | EFM_FLAG_PGSZERR | EFM_FLAG_PEPRTERR | EFM_FLAG_OTPWERR)
#define EFM_FLAG_ERR1                   (EFM_FLAG_ROWERR1 | EFM_FLAG_SWERR1 | EFM_FLAG_BLKERR1 | EFM_FLAG_COLERR1 | \
                                         EFM_FLAG_SPGMEND1 | EFM_FLAG_PGSZERR1 | EFM_FLAG_PEPRTERR1 | EFM_FLAG_OTPWERR1)
#define EFM_FLAG0_ALL                   (EFM_FLAG_ERR | EFM_FLAG_OPTEND | EFM_FLAG_RDY)
#define EFM_FLAG1_ALL                   (EFM_FLAG_ERR1 | EFM_FLAG_OPTEND1 | EFM_FLAG_RDY1)
#define EFM_FLAG_ALL                    (EFM_FLAG0_ALL | EFM_FLAG1_ALL)

#define EFM_FLAG_ECC                    (EFM_FLAG_ECC_OVF0)
#define EFM_FLAG_ECC1                   (EFM_FLAG_ECC_OVF1)

/**
 * @}
 */

/**
 * @defgroup EFM_Interrupt_Sel EFM Interrupt Selection
 * @{
 */
#define EFM_INT_PEERR                   (EFM_FITE_PEERRITE)     /*!< Program/erase error Interrupt source    */
#define EFM_INT_OPTEND                  (EFM_FITE_OPTENDITE)    /*!< End of EFM operation Interrupt source   */
#define EFM_INT_COLERR                  (EFM_FITE_COLERRITE)    /*!< Read collide error Interrupt source     */
#define EFM_INT_ALL                     (EFM_FITE_PEERRITE | EFM_FITE_OPTENDITE | EFM_FITE_COLERRITE)
/**
 * @}
 */

/**
 * @defgroup EFM_Keys EFM Keys
 * @{
 */
#define EFM_REG_UNLOCK_KEY1             (0x0123UL)
#define EFM_REG_UNLOCK_KEY2             (0x3210UL)
#define EFM_REG_LOCK_KEY                (0UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Sector_Size EFM Sector Size
 * @{
 */
#define EFM_SECTOR_SIZE                 (0x2000UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Sector_Address EFM Sector Address
 * @{
 */
#define EFM_SECTOR_ADDR(x)          (uint32_t)(EFM_SECTOR_SIZE * (x))
/**
 * @}
 */

/**
 * @defgroup EFM_OTP_Enable_Address EFM OTP Enable Address
 * @{
 */
#define EFM_OTP_ENABLE_ADDR             (0x03001FF0UL)    /*!< OTP Enable address */
/**
 * @}
 */

/**
 * @defgroup EFM_OTP_Unlock_Key EFM OTP Unlock Key
 * @{
 */
#define EFM_OTP_UNLOCK_KEY1             (0x10325476UL)
#define EFM_OTP_UNLOCK_KEY2             (0xEFCDAB89UL)
/**
 * @}
 */

/**
 * @defgroup EFM_OTP_Base_Address EFM Otp Base Address
 * @{
 */
#define EFM_OTP_BASE1_ADDR              (0x00000000UL)
#define EFM_OTP_BASE1_SIZE              (8*1024UL)
#define EFM_OTP_BASE1_OFFSET            (0UL)
#define EFM_OTP_BASE2_ADDR              (0x03000000UL)
#define EFM_OTP_BASE2_SIZE              (16UL)
#define EFM_OTP_BASE2_OFFSET            (16UL)
#define EFM_OTP_BASE3_ADDR              (0x03000600UL)
#define EFM_OTP_BASE3_SIZE              (512UL)
#define EFM_OTP_BASE3_OFFSET            (112UL)
#define EFM_OTP_BASE4_ADDR              (0x03000800UL)
#define EFM_OTP_BASE4_SIZE              (2*1024UL)
#define EFM_OTP_BASE4_OFFSET            (113UL)
#define EFM_OTP_BASE5_ADDR              (0x03001000UL)
#define EFM_OTP_BASE5_SIZE              (256UL)
#define EFM_OTP_BASE5_OFFSET            (114UL)
#define EFM_OTP_BASE6_ADDR              (0x03001400UL)
#define EFM_OTP_BASE6_SIZE              (1*1024UL)
#define EFM_OTP_BASE6_OFFSET            (118UL)
#define EFM_OTP_BASE7_ADDR              (0x0300E000UL)
#define EFM_OTP_BASE7_SIZE              (8*1024UL)
#define EFM_OTP_BASE7_OFFSET            (119UL)
#define EFM_OTP_LOCK_ADDR               (0x03001800UL)
/**
 * @}
 */

/**
 * @defgroup EFM_OTP_Address EFM Otp Address
 * @{
 */
#define EFM_OTP_BLOCK_IDX_MAX           (119UL)
#define EFM_OTP_BLOCK_BASE_ADDR_INVALID (0xFFFFFFFFUL)
#define EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx, BaseAddr, BaseOffset, BaseSize) \
    (BaseAddr + (((BlockIdx) - BaseOffset) * BaseSize))
#define EFM_OTP_BLOCK_ADDR(BlockIdx)             \
(\
    (BlockIdx < EFM_OTP_BASE2_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE1_ADDR,EFM_OTP_BASE1_OFFSET,EFM_OTP_BASE1_SIZE): \
    (BlockIdx < EFM_OTP_BASE3_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE2_ADDR,EFM_OTP_BASE2_OFFSET,EFM_OTP_BASE2_SIZE): \
    (BlockIdx < EFM_OTP_BASE4_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE3_ADDR,EFM_OTP_BASE3_OFFSET,EFM_OTP_BASE3_SIZE): \
    (BlockIdx < EFM_OTP_BASE5_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE4_ADDR,EFM_OTP_BASE4_OFFSET,EFM_OTP_BASE4_SIZE): \
    (BlockIdx < EFM_OTP_BASE6_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE5_ADDR,EFM_OTP_BASE5_OFFSET,EFM_OTP_BASE5_SIZE): \
    (BlockIdx < EFM_OTP_BASE7_OFFSET) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE6_ADDR,EFM_OTP_BASE6_OFFSET,EFM_OTP_BASE6_SIZE): \
    (BlockIdx <= EFM_OTP_BLOCK_IDX_MAX) ? \
    EFM_OTP_CALC_BLOCK_BASE_ADDR(BlockIdx,EFM_OTP_BASE7_ADDR,EFM_OTP_BASE7_OFFSET,EFM_OTP_BASE7_SIZE): \
    EFM_OTP_BLOCK_BASE_ADDR_INVALID\
)
/**
 * @}
 */

/**
 * @defgroup EFM_OTP_Lock_Address EFM Otp Lock_address
 *          x: otp block index, range from 0 to @ref EFM_OTP_BLOCK_IDX_MAX
 * @{
 */
#define EFM_OTP_BLOCK_LOCKADDR(x)       (EFM_OTP_LOCK_ADDR + EFM_PGM_UNIT_BYTES * (x))   /*!< OTP block x  lock address */
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_Reg_Write_Protection Write Protection Keys For EFM Remap Registers
 * @{
 */
#define EFM_REMAP_REG_LOCK_KEY          (0x0000UL)
#define EFM_REMAP_REG_UNLOCK_KEY1       (0x0123UL)
#define EFM_REMAP_REG_UNLOCK_KEY2       (0x3210UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_State EFM remap function state
 * @{
 */
#define EFM_REMAP_OFF                   (0UL)
#define EFM_REMAP_ON                    (EFM_MMF_REMCR_EN)
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_Size EFM remap size definition
 * @note refer to chip user manual for details size spec.
 * @{
 */
#define EFM_REMAP_4K                    (12UL)
#define EFM_REMAP_8K                    (13UL)
#define EFM_REMAP_16K                   (14UL)
#define EFM_REMAP_32K                   (15UL)
#define EFM_REMAP_64K                   (16UL)
#define EFM_REMAP_128K                  (17UL)
#define EFM_REMAP_256K                  (18UL)
#define EFM_REMAP_512K                  (19UL)
#define EFM_REMAP_SIZE_MAX              EFM_REMAP_512K
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_Index EFM remap index
 * @{
 */
#define EFM_REMAP_IDX0                  (0U)
#define EFM_REMAP_IDX1                  (1U)
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_Base_Addr EFM remap base address
 * @{
 */
#define EFM_REMAP_BASE_ADDR0            (0x02000000UL)
#define EFM_REMAP_BASE_ADDR1            (0x02080000UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Remap_Region EFM remap ROM/RAM region
 * @{
 */
#define EFM_REMAP_ROM_END_ADDR          EFM_END_ADDR

#define EFM_REMAP_RAM_START_ADDR        (0x1FFF8000UL)
#define EFM_REMAP_RAM_END_ADDR          (0x1FFFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup EFM_Protect_Level EFM protect level
 * @{
 */
#define EFM_PROTECT_LEVEL1              (1UL << 0UL)
#define EFM_PROTECT_LEVEL2              (1UL << 1UL)
#define EFM_PROTECT_LEVEL3              (1UL << 2UL)
#define EFM_PROTECT_LEVEL_ALL           (EFM_PROTECT_LEVEL1 | EFM_PROTECT_LEVEL2 | EFM_PROTECT_LEVEL3)
/**
 * @}
 */

/**
 * @defgroup EFM_Protect_Security_Addr EFM protect security address define
 * @{
 */
#define EFM_SECURITY_ADDR               (0x0300E070UL)
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Mode EFM ECC Mode
 *  @{
 */
/**
 * @note
 @verbatim
 * ------------------------------------------------------------------------
 *  ECC Mode | Error type  | Correct or | Set error flag  | Generate NMI |
 *           |             | detect?    | ?               | or reset ?   |
 * ------------------------------------------------------------------------
 *  INVD       1-bit-error    no           no               no
 *             2-bit-error    no           no               no
 * ------------------------------------------------------------------------
 *  MD1        1-bit-error    correct      no               no
 *             2-bit-error    detect       yes              yes
 * ------------------------------------------------------------------------
 *  MD2        1-bit-error    correct      yes              no
 *             2-bit-error    detect       yes              yes
 * ------------------------------------------------------------------------
 *  MD3        1-bit-error    correct      yes              yes
 *             2-bit-error    detect       yes              yes
 * ------------------------------------------------------------------------
 @endverbatim
 */
#define EFM_ECC_MD_INVD                 (0UL)
#define EFM_ECC_MD1                     (EFM_CKCR_F0ECCMOD_0)
#define EFM_ECC_MD2                     (EFM_CKCR_F0ECCMOD_1)
#define EFM_ECC_MD3                     (EFM_CKCR_F0ECCMOD)
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_CKCR_Key EFM ECC CKCR unlock/lock key
 * @{
 */
#define EFM_ECC_CKCR_UNLOCK_KEY         (0x77U)
#define EFM_ECC_CKCR_LOCK_KEY           (0x76U)
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Exception_Type EFM ECC Exception Type
 * @{
 */
#define EFM_ECC_EXP_TYPE_NMI            (0UL)                                   /*!< MCU enter NMI handle while ECC error occurs    */
#define EFM_ECC_EXP_TYPE_RST            (EFM_CKCR_F0ECCOAD)                     /*!< MCU reset while ECC error occurs               */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Blank_Read_Mode EFM ECC bank read mode
 * @{
 */
#define EFM_ECC_BLK_RD_NO_ERR           (0UL)                                   /*!< EFM ECC no error while blank read              */
#define EFM_ECC_BLK_RD_ALG              (EFM_FWMC_BLKECCRSEL)                   /*!< EFM ECC error by algorithm while blank read    */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Blank_Write_Mode EFM ECC bank write mode
 * @{
 */
#define EFM_ECC_BLK_WR_HW_REG           (0UL)                                   /*!< EFM ECC data is generated by hardware or the value of ECCDR register while blank write */
#define EFM_ECC_BLK_WR_1FF              (EFM_FWMC_BLKECCWSEL)                   /*!< EFM ECC data is 0x1FF while blank write                 */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Flag EFM ECC Flag
 * @{
 */
#define EFM_ECC_CHIP0_1BIT_ERR          (EFM_CKSR_F0_1ERR)                      /*!< EFM chip 0 ECC 1-bit error flag */
#define EFM_ECC_CHIP0_2BIT_ERR          (EFM_CKSR_F0_2ERR)                      /*!< EFM chip 0 ECC 2-bit error flag */
#define EFM_ECC_CHIP0_ALL               (EFM_CKSR_F0_1ERR | EFM_CKSR_F0_2ERR)   /*!< EFM chip 0 ECC all error flag   */

#define EFM_ECC_CHIP1_1BIT_ERR          (EFM_CKSR_F1_1ERR)                      /*!< EFM chip 1 ECC 1-bit error flag */
#define EFM_ECC_CHIP1_2BIT_ERR          (EFM_CKSR_F1_2ERR)                      /*!< EFM chip 1 ECC 2-bit error flag */
#define EFM_ECC_CHIP1_ALL               (EFM_CKSR_F1_1ERR | EFM_CKSR_F1_2ERR)   /*!< EFM chip 1 ECC all error flag   */
#define EFM_ECC_FLAG_ALL                (EFM_ECC_CHIP0_ALL | EFM_ECC_CHIP1_ALL) /*!< EFM ECC all error flag          */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Data_Sel EFM ECC data selection while read flag or program flash
 * @{
 */
#define EFM_ECC_DATA_RD_WR_HW           (0UL)                                   /*!< Auto to verify the ECC data by hardware while read flash, Auto to generate ECC data by hardware while program flash  */
#define EFM_ECC_DATA_RD_REG             (EFM_FECCCR_VDIS)                       /*!< The ECC data will be read to ECCDR.DRD while read flash   */
#define EFM_ECC_DATA_WR_REG             (EFM_FECCCR_GDIS)                       /*!< Use the ECCDR.DWD as the ECC data while program flash     */
#define EFM_ECC_DATA_RD_WR_REG          (EFM_FECCCR_VDIS | EFM_FECCCR_GDIS)     /*!< ECC data by ECCDR register whether read or program flash  */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Read_mode EFM ECC read mode
 * @{
 */
#define EFM_ECC_RD_NORMAL               (0U)                                    /*!< EFM normal read                    */
#define EFM_ECC_RD_ERR_INJECT_CHIP0     (EFM_EIEN_F0_EIEN)                      /*!< EFM chip 0 error inject read mode  */
#define EFM_ECC_RD_ERR_INJECT_CHIP1     (EFM_EIEN_F1_EIEN)                      /*!< EFM chip 1 error inject read mode  */
#define EFM_ECC_RD_ERR_INJECT_ALL       (EFM_ECC_RD_ERR_INJECT_CHIP0 | EFM_ECC_RD_ERR_INJECT_CHIP1)
/**
 * @}
 */

/**
 * @defgroup EFM_Blank_Read_Mode   EFM margin read mode
 * @{
 */
#define EFM_BLK_RD_MD_CHIP0             (EFM_FRMC2_BRM0)                        /*!< EFM chip0 blank read, check whether all '1' */
#define EFM_BLK_RD_MD_CHIP1             (EFM_FRMC2_BRM1)                        /*!< EFM chip1 blank read, check whether all '1' */
#define EFM_RD_MD_BLANK                 (EFM_BLK_RD_MD_CHIP0 | EFM_BLK_RD_MD_CHIP1)
/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup EFM_Global_Functions
 * @{
 */

/**
 * @brief  EFM Protect Unlock.
 * @param  None
 * @retval None
 */

__STATIC_INLINE void EFM_REG_Unlock(void)
{
    WRITE_REG32(CM_EFM->FAPRT, EFM_REG_UNLOCK_KEY1);
    WRITE_REG32(CM_EFM->FAPRT, EFM_REG_UNLOCK_KEY2);
}

/**
 * @brief  EFM Protect Lock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_REG_Lock(void)
{
    WRITE_REG32(CM_EFM->FAPRT, EFM_REG_LOCK_KEY);
}

/**
 * @brief  EFM remap Unlock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_REMAP_Unlock(void)
{
    WRITE_REG32(CM_EFM->MMF_REMPRT, EFM_REMAP_REG_UNLOCK_KEY1);
    WRITE_REG32(CM_EFM->MMF_REMPRT, EFM_REMAP_REG_UNLOCK_KEY2);
}

/**
 * @brief  EFM remap Lock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_REMAP_Lock(void)
{
    WRITE_REG32(CM_EFM->MMF_REMPRT, EFM_REMAP_REG_LOCK_KEY);
}

/**
 * @brief  EFM OTP reg unlock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_OTP_REG_Unlock(void)
{
    WRITE_REG32(CM_EFM->KEY2, EFM_OTP_UNLOCK_KEY1);
    WRITE_REG32(CM_EFM->KEY2, EFM_OTP_UNLOCK_KEY2);
}

/**
 * @brief  EFM OTP reg Lock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_OTP_REG_Lock(void)
{
    SET_REG32_BIT(CM_EFM->FWMC, EFM_FWMC_KEY2LOCK);
}

/**
 * @brief  EFM ECC CKCR reg unlock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_ECC_CKCR_Unlock(void)
{
    WRITE_REG32(CM_EFM->CKPR, EFM_ECC_CKCR_UNLOCK_KEY);
}

/**
 * @brief  EFM ECC CKCR reg lock.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void EFM_ECC_CKCR_Lock(void)
{
    WRITE_REG32(CM_EFM->CKPR, EFM_ECC_CKCR_LOCK_KEY);
}

/************************************ BASE ************************************/
void EFM_Cmd(uint32_t u32Chip, en_functional_state_t enNewState);
void EFM_FWMC_Cmd(en_functional_state_t enNewState);
void EFM_SetBusStatus(uint32_t u32Status);
void EFM_IntCmd(uint32_t u32EfmInt, en_functional_state_t enNewState);
en_flag_status_t EFM_GetAnyStatus(uint32_t u32Flag);
en_flag_status_t EFM_GetStatus(uint32_t u32Flag);
void EFM_ClearStatus(uint32_t u32Flag);
int32_t EFM_SetWaitCycle(uint32_t u32WaitCycle);
uint32_t EFM_GetWaitCycle(void);
int32_t EFM_SetOperateMode(uint32_t u32Mode);
void EFM_GetUID(stc_efm_unique_id_t *pstcUID);

uint32_t EFM_GetCID(void);

uint8_t EFM_GetWaferID(void);
void EFM_GetLocation(stc_efm_location_t *pstcLocation);
uint64_t EFM_GetLotID(void);

void EFM_CacheRamReset(en_functional_state_t enNewState);
void EFM_CacheCmd(uint32_t u32Cache, en_functional_state_t enNewState);

/************************************ SWAP ************************************/
int32_t EFM_SwapCmd(en_functional_state_t enNewState);
__EFM_FUNC en_flag_status_t EFM_GetSwapStatus(void);
void EFM_RealTimeSwapCmd(en_functional_state_t enNewState);

/************************************ OTP *************************************/
__EFM_FUNC en_flag_status_t EFM_OTP_GetStatus(void);
int32_t EFM_OTP_Enable(void);
int32_t EFM_OTP_Lock(uint32_t u32BlockStartIdx, uint16_t u16Count);

/************************************ ERASE ***********************************/
int32_t EFM_SectorErase(uint32_t u32Addr);
int32_t EFM_SequenceSectorErase(uint32_t u32StartSectorNum, uint16_t u16Count);
int32_t EFM_ChipErase(uint32_t u32Chip);

/************************************ WRITE ***********************************/
int32_t EFM_Program(uint32_t u32Addr, const uint8_t *pu8DataSource, uint32_t u32ByteLen);
int32_t EFM_SequenceProgram(uint32_t u32Addr, const uint8_t *pu8DataSource, uint32_t u32ByteLen);
int32_t EFM_ProgramUnit(uint32_t u32Addr, const uint32_t *pu32Buf);

/************************************ READ ************************************/
int32_t EFM_ReadByte(uint32_t u32Addr, uint8_t *pu8ReadBuf, uint32_t u32ByteLen);
int32_t EFM_BlankRead(uint32_t u32Addr, uint32_t u32ByteLen, uint32_t u32Mode, uint32_t *pu32ErrorAddr);

/************************************ ECC *************************************/
void EFM_ECC_Config(uint32_t u32Chip, uint32_t u32Mode, uint32_t u32ExceptionType);
en_flag_status_t EFM_ECC_GetStatus(uint32_t u32Flag);
void EFM_ECC_ClearStatus(uint32_t u32Flag);
void EFM_ECC_SetMode(uint32_t u32Chip, uint32_t u32Mode);
void EFM_ECC_SetExceptionType(uint32_t u32Chip, uint32_t u32ExceptionType);
void EFM_ECC_SetReadMode(uint32_t u32Mode);
void EFM_ECC_SetBlankRead(uint32_t u32Mode);
void EFM_ECC_SetBlankWrite(uint32_t u32Mode);
void EFM_ECC_GetErrorAddr(uint32_t u32Chip, uint32_t *pu32EccAddr);
void EFM_ECC_ErrorInjectBitCmd(uint32_t u32Chip, uint16_t u16BitPos, en_functional_state_t enNewState);
void EFM_ECC_ClearErrorAddr(uint32_t u32Chip);
void EFM_ECC_SetEccDataMode(uint32_t u32Chip, uint32_t u32Mode);
void EFM_ECC_Write(uint32_t u32Chip, uint32_t u32EccData);
uint32_t EFM_ECC_Read(uint32_t u32Chip);
/************************************ PROTECT *********************************/
void EFM_Protect_Enable(uint8_t u8Level);
int32_t EFM_WriteSecurityCode(const uint8_t *pu8Buf, uint32_t u32ByteLen);
void EFM_SectorProtectRegLock(uint32_t u32RegLock);
void EFM_SingleSectorOperateCmd(uint32_t u32SectorNum, en_functional_state_t enNewState);
void EFM_SequenceSectorOperateCmd(uint32_t u32StartSectorNum, uint16_t u16Count, en_functional_state_t enNewState);

/************************************ REMAP ***********************************/
int32_t EFM_REMAP_StructInit(stc_efm_remap_init_t *pstcEfmRemapInit);
int32_t EFM_REMAP_Init(uint8_t u8RemapIdx, stc_efm_remap_init_t *pstcEfmRemapInit);
void EFM_REMAP_DeInit(void);
void EFM_REMAP_Cmd(uint8_t u8RemapIdx, en_functional_state_t enNewState);
void EFM_REMAP_SetAddr(uint8_t u8RemapIdx, uint32_t u32Addr);
void EFM_REMAP_SetSize(uint8_t u8RemapIdx, uint32_t u32Size);

/**
 * @}
 */

#endif /* LL_EFM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_EFM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
