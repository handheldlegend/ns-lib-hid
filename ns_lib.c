/**
 * @file ns_lib.c
 * @brief Weak default implementations of NS-LIB-HID console callback hooks.
 *
 * Copyright (c) 2026 Mitchell Cairns, Hand Held Legend, LLC.
 *
 * Licensed under the Creative Commons Attribution-NonCommercial 4.0 International License
 * (CC BY-NC 4.0). Non-commercial use with attribution; commercial licensing from Hand Held Legend.
 * Full terms: https://creativecommons.org/licenses/by-nc/4.0/legalcode — see LICENSING.md in this folder
 *
 * SPDX-License-Identifier: CC-BY-NC-4.0
 *
 * TRADEMARK AND AFFILIATION DISCLAIMER:
 * This library is not affiliated, associated, authorized, endorsed by, or in any way officially
 * connected with Nintendo Co., Ltd., or any of its subsidiaries or its affiliates. Nintendo and
 * related marks are trademarks of their respective owners.
 */

#include "ns_lib.h"
#include "ns_lib_protocol.h"

__attribute__((weak)) void ns_set_haptic_indices_cb(const ns_lib_haptic_raw_sample_s *pairs, uint8_t pair_count)
{
    (void)pairs;
    (void)pair_count;
}

__attribute__((weak)) void ns_set_led_cb(int player_leds)
{
    (void)player_leds;
}

__attribute__((weak)) void ns_set_power_cb(uint8_t shutdown)
{
    (void)shutdown;
}

__attribute__((weak)) void ns_set_usbpair_cb(ns_usbpair_s pairing_data)
{
    (void)pairing_data;
}

__attribute__((weak)) void ns_set_imumode_cb(ns_imu_mode_t imu_mode)
{
    (void)imu_mode;
}


__attribute__((weak)) void ns_get_powerstatus_cb(ns_powerstatus_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->bat_lvl = 4;
    out->power_source = 1;
    out->connection = 1;
    out->charging = 0;
    out->reserved = 0;
}

__attribute__((weak)) void ns_get_imu_raw_cb(ns_gyrodata_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
}

__attribute__((weak)) void ns_get_inputdata_cb(ns_inputdata_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->ls_x = 2048;
    out->ls_y = 2048;
    out->rs_x = 2048;
    out->rs_y = 2048;
}

__attribute__((weak)) void ns_linkkey_get_cb(uint8_t *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, 16);
}

__attribute__((weak)) void ns_devmac_get_cb(uint8_t *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, 6);
}

__attribute__((weak)) void ns_hostmac_get_cb(uint8_t *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, 6);
}

__attribute__((weak)) void ns_get_imu_quaternion_cb(ns_quaternion_s *out)
{
    if (!out)
    {
        return;
    }
    static ns_quaternion_s quat_state = {.raw = {0.f, 0.f, 0.f, 1.f}};
    *out = quat_state;
}

ns_config_status_t ns_api_init(const ns_device_config_s *cfg)
{
    ns_config_status_t st = ns_device_config_set(cfg);
    if (st != NS_CONFIG_OK)
    {
        return st;
    }
    ns_analog_calibration_init();
    ns_haptics_init();
    return NS_CONFIG_OK;
}

bool ns_api_generate_inputreport(uint8_t data[64])
{
    if (data == NULL)
    {
        return false;
    }
    return ns_protocol_generate_inputreport(data);
}

ns_config_status_t ns_lib_init(const ns_device_config_s *cfg)
{
    return ns_api_init(cfg);
}

void ns_api_output_tunnel(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0u)
    {
        return;
    }
    ns_protocol_process_outputreport(data, len);
}

/*
 * Platform hooks (declared in ns_lib_api.h): weak definitions — firmware should supply strong replacements.
 */
__attribute__((weak)) void ns_get_time_ms(uint64_t *ms)
{
    static uint64_t t;
    if (!ms)
    {
        return;
    }
    t++;
    *ms = t;
}

__attribute__((weak)) uint8_t ns_get_random_u8(void)
{
    static uint16_t s = 0xACE1u;
    s = (uint16_t)(s * 1103515245u + 12345u);
    return (uint8_t)(s >> 8);
}
