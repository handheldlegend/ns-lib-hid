/**
 * @file ns_lib_config.c
 * @brief Global NS-LIB-HID device configuration storage and helpers (@ref ns_lib_config.h).
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

#include <stddef.h>
#include <string.h>

#include "ns_lib_config.h"
#include "ns_lib_motion.h"

static ns_device_config_s s_ns_device_config;
/** Updated only from ns_device_config_set / ns_device_config_reset — avoids re-validation on hot paths. */
static uint8_t s_ns_config_ready;
static const float NS_CFG_DEFAULT_GYRO_FS_DPS = 2000.0f;

void ns_config_get_fw_version(uint8_t *upper, uint8_t *lower)
{
    *upper = 0x04;
    *lower = 0x33;
}

void ns_config_get_devtype_bytes(uint8_t *id_hi, uint8_t *id_lo, uint8_t *color_byte,
                                           uint8_t *snes_region_byte)
{
    *color_byte = 0x01;
    *snes_region_byte = 0x00;

    switch (s_ns_device_config.type)
    {
    case NS_DEVTYPE_JOYCON_L:
        *id_hi = 0x01;
        *id_lo = 0x02;
        return;
    case NS_DEVTYPE_JOYCON_R:
        *id_hi = 0x02;
        *id_lo = 0x02;
        return;
    case NS_DEVTYPE_N64:
        *id_hi = 0x0C;
        *id_lo = 0x11;
        return;
    case NS_DEVTYPE_SNES_NA:
        *id_hi = 0x0B;
        *id_lo = 0x02;
        *color_byte = 0x02;
        *snes_region_byte = 0x00;
        return;
    case NS_DEVTYPE_SNES_JP:
        *id_hi = 0x0B;
        *id_lo = 0x02;
        *color_byte = 0x02;
        *snes_region_byte = 0x01;
        return;
    case NS_DEVTYPE_SNES_EU:
        *id_hi = 0x0B;
        *id_lo = 0x02;
        *color_byte = 0x02;
        *snes_region_byte = 0x02;
        return;
    case NS_DEVTYPE_FAMI:
        *id_hi = 0x07;
        *id_lo = 0x02;
        return;
    case NS_DEVTYPE_NES:
        *id_hi = 0x09;
        *id_lo = 0x02;
        return;
    case NS_DEVTYPE_MEGADRIVE:
    case NS_DEVTYPE_GENESIS:
        *id_hi = 0x0D;
        *id_lo = 0x02;
        return;
    case NS_DEVTYPE_PROCON:
    case NS_DEVTYPE_UNDEFINED:
    default:
        *id_hi = 0x03;
        *id_lo = 0x02;
        return;
    }
}

ns_config_status_t ns_config_validate(const ns_device_config_s *cfg)
{
    if (!cfg)
    {
        return NS_CONFIG_INVALID_ARG;
    }
    if (cfg->type == NS_DEVTYPE_UNDEFINED)
    {
        return NS_CONFIG_NOT_CONFIGURED;
    }
    if (cfg->transport == NS_TRANSPORT_UNDEFINED)
    {
        return NS_CONFIG_NOT_CONFIGURED;
    }
    return NS_CONFIG_OK;
}

ns_config_status_t ns_config_set(const ns_device_config_s *cfg)
{
    ns_config_status_t st = ns_config_validate(cfg);
    if (st != NS_CONFIG_OK)
    {
        return st;
    }
    memcpy(&s_ns_device_config, cfg, sizeof(s_ns_device_config));
    if (s_ns_device_config.gyro_full_scale_dps <= 0.0f)
    {
        s_ns_device_config.gyro_full_scale_dps = NS_CFG_DEFAULT_GYRO_FS_DPS;
    }
    s_ns_device_config.gyro_rad_per_lsb =
        ns_motion_calculate_rps(s_ns_device_config.gyro_full_scale_dps);
    s_ns_config_ready = 1u;
    return NS_CONFIG_OK;
}

void ns_config_reset(void)
{
    memset(&s_ns_device_config, 0, sizeof(s_ns_device_config));
    s_ns_config_ready = 0u;
}

void ns_config_get(ns_device_config_s *out)
{
    if (!out)
    {
        return;
    }
    memcpy(out, &s_ns_device_config, sizeof(*out));
}

int ns_config_get_ready(void)
{
    return (int)s_ns_config_ready;
}

float ns_config_get_gyro_rpl(void)
{
    return s_ns_device_config.gyro_rad_per_lsb;
}

void ns_config_get_device_mac(uint8_t out[6])
{
    memcpy(out, s_ns_device_config.device_mac, 6);
}

void ns_config_get_host_mac(uint8_t out[6])
{
    memcpy(out, s_ns_device_config.host_mac, 6);
}

void ns_config_set_host_mac(uint8_t out[6])
{
    memcpy(s_ns_device_config.host_mac, out, 6);
}

ns_transport_t ns_config_get_transport(void)
{
    return s_ns_device_config.transport;
}
