// SPDX-License-Identifier: MIT
/**
 * @file    test_demo_storage.c
 * @brief   Exercise real storage transactions, data-pool exchanges, geometry and ECC.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Exercise real storage transactions, data-pool exchanges, geometry and ECC.
 *          - Keep operations bounded and report invalid requests.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Task-context API; not ISR-safe.
 *          - Hardware access is confined to the platform BSP.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "fake_flash.h"
#include "fal_cfg.h"
#include "demo_storage_service.h"
#include "demo_storage_business.h"
#include "demo_storage_record.h"
#include "flash_port.h"
#include "flash_integrity.h"
#include "bch4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks = 0u; /* Ordered assertion log. */
static void check(const char *label, int64_t expected, int64_t actual)
{
    checks++;
    (void)printf("CHECK %04u %-40s expected=%lld actual=%lld %s\n",
        checks, label, (long long)expected, (long long)actual, expected == actual ? "PASS" : "FAIL");
    if (expected != actual) { exit(1); }
}
static demo_storage_snapshot_t drain(void)
{
    demo_storage_snapshot_t snapshot = {0}; /* Real pool publication. */
    uint32_t step = 0u; /* Bounded execution. */
    do
    {
        (void)fal_runtime_process(&g_demo_fal_runtime);
        demo_storage_service_process();
        if (demo_storage_business_read(&snapshot) == 0u) { exit(2); }
        step++;
    } while ((snapshot.busy != 0u) && (step < 4000u));
    check("transaction completed", 0, snapshot.busy);
    (void)printf("OUTPUT steps=%lu result=%ld RAM=%lu,%lu,%lu seq=%lu,%lu\n",
        (unsigned long)step, (long)snapshot.result, (unsigned long)snapshot.params.input_mv,
        (unsigned long)snapshot.params.gain_milli, (unsigned long)snapshot.params.threshold_mv,
        (unsigned long)snapshot.devices[0].sequence, (unsigned long)snapshot.devices[1].sequence);
    return snapshot;
}
static void reset_case(const char *name)
{
    (void)printf("\nCASE %s\nRESET fake BSP, pool defaults, service\n", name);
    fake_reset(); (void)fal_runtime_init(&g_demo_fal_runtime); demo_storage_service_init(); (void)drain();
    check("boot performs no write", 0, fake_writes());
}
static demo_storage_snapshot_t command(uint8_t device, DEMO_STORAGE_COMMAND_E cmd)
{
    (void)printf("INPUT device=%u command=%u via business_request\n", (unsigned)device, (unsigned)cmd);
    check("request accepted", 1, demo_storage_business_request(device, cmd));
    return drain();
}
static void configure(uint32_t value)
{
    demo_storage_params_t p = {.input_mv = value, .gain_milli = 2000u, .threshold_mv = 5000u}; /* Input mV. */
    (void)printf("INPUT configure input_mv=%lu gain_milli=2000 threshold_mv=5000\n", (unsigned long)value);
    check("configure accepted", 1, demo_storage_business_configure(&p));
}
static void transactions(void)
{
    reset_case("two media save/load, busy freeze, defaults, ring and cross-block scratch");
    for (uint8_t d = 0u; d < 2u; d++)
    {
        demo_storage_snapshot_t s = {0}; /* Transaction results. */
        demo_storage_params_t invalid = {1u, 1u, 1u}; /* Busy-time replacement attempt. */
        configure(1234u + d);
        check("save accepted", 1, demo_storage_business_request(d, DEMO_STORAGE_SAVE));
        check("concurrent request rejected", 0, demo_storage_business_request(d, DEMO_STORAGE_SCAN));
        check("busy RAM edit rejected", 0, demo_storage_business_configure(&invalid));
        s = drain(); check("save result", 0, s.result); check("initial sequence", 1, s.devices[d].sequence);
        configure(4567u);
        s = command(d, DEMO_STORAGE_LOAD); check("load restores data pool", 1234u + d, s.params.input_mv);
        configure(6789u); s = command(d, DEMO_STORAGE_SAVE); check("A/B next sequence", 2, s.devices[d].sequence);
        s = command(d, DEMO_STORAGE_TEST); check("cross-block test", 0, s.result);
        s = command(d, DEMO_STORAGE_LOAD); check("scratch preserves parameters", 6789, s.params.input_mv);
        for (uint32_t i = 1u; i <= 6u; i++)
        {
            configure(i); s = command(d, DEMO_STORAGE_LOG); check("log append result", 0, s.result);
            check("ring sequence", i, s.devices[d].log_sequence);
        }
        s = command(d, DEMO_STORAGE_SCAN); check("log rescan latest input", 6, s.devices[d].last_log.input_mv);
        { uint32_t count = fake_writes(); /* Defaults only changes RAM. */
          s = command(d, DEMO_STORAGE_DEFAULTS); check("defaults input", 1000, s.params.input_mv);
          check("defaults no erase/program", count, fake_writes()); }
    }
    (void)fal_runtime_init(&g_demo_fal_runtime); demo_storage_service_init();
    { demo_storage_snapshot_t s = drain(); check("reboot selects NOR parameters", 6789, s.params.input_mv); }
    check("protected prefix never written", 0, fake_prefix_writes());
    {
        demo_storage_params_t invalid = {100001u, 1000u, 1500u}; /* Invalid source-layer value. */
        uint32_t count = fake_writes(); /* No mutation for rejected payload. */
        check("RAM pool accepts typed value", 1, demo_storage_business_configure(&invalid));
        demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_SAVE);
        check("storage validates semantic range", FAL_RESULT_INVALID_ARGUMENT, s.result);
        check("invalid payload never erased", count, fake_writes());
    }
}
static void failures(void)
{
    reset_case("failed commit keeps old copy and incomplete scan blocks overwrite");
    configure(1111u); (void)command(0u, DEMO_STORAGE_SAVE);
    configure(2222u); fake_program_fault(0u, 0x1F9000u + 256u);
    { demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_SAVE); check("commit error propagated", FAL_RESULT_DRIVER_ERROR, s.result); }
    fake_program_fault(0u, UINT32_MAX);
    { demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_LOAD); check("uncommitted B ignored", 1111, s.params.input_mv); }
    configure(3333u); (void)command(0u, DEMO_STORAGE_SAVE);
    fake_flip(0u, 0x1F9000u + 16u);
    { demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_LOAD); check("CRC bad B falls back A", 1111, s.params.input_mv); }
    fake_read_fault(0u, 0x1F8000u);
    { uint32_t count = fake_writes(); demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_SAVE);
      check("unreadable scan blocks SAVE", -23, s.result); check("no destructive retry", count, fake_writes()); }
    fake_read_fault(0u, UINT32_MAX);
    reset_case("NOR absent does not prevent NAND use");
    fake_present(0u, 0u);
    { demo_storage_snapshot_t s = command(0u, DEMO_STORAGE_SCAN); check("absent device result", -22, s.result); }
    configure(4444u);
    { demo_storage_snapshot_t s = command(1u, DEMO_STORAGE_SAVE); check("independent NAND save", 0, s.result); }
}

static void alignment(void)
{
    fal_t fal = {0}; /* Separate real FAL instance. */
    fal_device_cfg_t device = {0}; /* Real interface binding. */
    fal_zone_cfg_t zones[2] = {{.zone_id = 1u, .size = 0x7F00000u, .permissions = FAL_ZONE_PERMISSION_READ},
        {.zone_id = 2u, .size = 0x100000u, .permissions = FAL_ZONE_PERMISSION_ALL}};
    fal_cfg_t cfg = {.p_devices = &device, .device_count = 1u}; /* Local geometry validation. */
    uint8_t data[2048] = {0}; /* Full program unit. */
    reset_case("NAND write unit and readonly prefix");
    device = *demo_fal_device_get(DEMO_FAL_NAND);
    check("copy registered NAND device", 2, device.device_id);
    device.p_zones = zones; device.zone_count = 2u;
    check("FAL init", 0, fal_init(&fal, &cfg));
    check("unaligned length denied", FAL_RESULT_INVALID_ARGUMENT, fal_write(&fal, 2u, 0u, 1u, data));
    check("unaligned address denied", FAL_RESULT_INVALID_ARGUMENT, fal_write(&fal, 2u, 1u, 2048u, data));
    check("zero length succeeds", 0, fal_write(&fal, 2u, 1u, 0u, data));
    check("prefix write forbidden", FAL_RESULT_PERMISSION_DENIED, fal_write(&fal, 1u, 0u, 2048u, data));
    check("prefix erase forbidden", FAL_RESULT_PERMISSION_DENIED, fal_erase(&fal, 1u, 0u, 131072u));
    check("invalid requests never touch hardware", 0, fake_writes());
    device.program_unit_size = 3u;
    check("invalid unit geometry", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &cfg));
}
static void flip_bit(uint8_t *data, uint8_t *ecc, uint32_t bit)
{
    if (bit < 52u) { ecc[bit / 8u] ^= (uint8_t)(1u << (bit % 8u)); }
    else { bit -= 52u; data[bit / 8u] ^= (uint8_t)(1u << (bit % 8u)); }
}
static void integrity(void)
{
    uint8_t data[512], original[512], ecc[7], saved[7]; /* Independent payload and parity. */
    const uint8_t expected[7] = {0x71u,0x89u,0x13u,0x36u,0xCDu,0xB3u,0xF3u}; /* Python polynomial long division. */
    uint32_t random = 0x12345678u; /* Reproducible random sequence. */
    (void)printf("\nCASE BCH4 polynomial vector, every single bit, 1..4 bit errors, CRC\n");
    check("CRC32 check vector", UINT32_C(0xCBF43926), flash_integrity_crc32((const uint8_t *)"123456789", 9u));
    for (uint32_t i = 0u; i < 512u; i++) { original[i] = (uint8_t)i; }
    bch4_encode(original, saved);
    check("BCH parity independent vector", 0, memcmp(saved, expected, 7u));
    for (uint32_t bit = 0u; bit < 4148u; bit++)
    {
        (void)memcpy(data, original, 512u); (void)memcpy(ecc, saved, 7u); flip_bit(data, ecc, bit);
        check("single-bit correction", 1, bch4_correct(data, ecc));
        check("single-bit payload restored", 0, memcmp(data, original, 512u));
        check("single-bit parity restored", 0, memcmp(ecc, saved, 7u));
    }
    for (uint32_t trial = 0u; trial < 400u; trial++)
    {
        uint32_t bits[4] = {0}; /* Unique injected positions. */
        uint32_t count = trial % 4u + 1u; /* Error weight. */
        (void)memcpy(data, original, 512u); (void)memcpy(ecc, saved, 7u);
        for (uint32_t i = 0u; i < count; i++)
        {
            uint8_t duplicate = 0u; /* Reject collisions. */
            do
            {
                random = random * 1664525u + 1013904223u; bits[i] = random % 4148u; duplicate = 0u;
                for (uint32_t j = 0u; j < i; j++) { if (bits[i] == bits[j]) { duplicate = 1u; } }
            } while (duplicate != 0u);
            flip_bit(data, ecc, bits[i]);
        }
        check("multi-bit corrected count", count, bch4_correct(data, ecc));
        check("multi-bit payload restored", 0, memcmp(data, original, 512u));
        check("multi-bit parity restored", 0, memcmp(ecc, saved, 7u));
    }
    check("null ECC input rejected", -1, bch4_correct(NULL, ecc));
    for (uint32_t trial = 0u; trial < 100u; trial++)
    {
        int32_t result = 0; /* Decoder may reject or miscorrect beyond its guaranteed radius. */
        uint32_t start = trial * 37u; /* Five adjacent main-area bit errors. */
        (void)memcpy(data, original, 512u); (void)memcpy(ecc, saved, 7u);
        for (uint32_t i = 0u; i < 5u; i++) { flip_bit(data, ecc, 52u + start + i); }
        result = bch4_correct(data, ecc);
        check("five-bit corruption rejected by BCH/CRC", 1,
            (result < 0) || (flash_integrity_crc32(data, 512u) != flash_integrity_crc32(original, 512u)));
    }
    check("sequence wrap newer", 1, demo_storage_record_newer(1u, UINT32_MAX));
    check("old sequence not newer", 0, demo_storage_record_newer(UINT32_MAX, 1u));
}
int main(void)
{
    transactions(); failures(); alignment(); integrity();
    (void)printf("SUMMARY checks=%u passed=%u failed=0\n", checks, checks);
    return 0;
}
