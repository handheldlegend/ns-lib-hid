/**
 * @file ns_lib_protocol.h
 * @brief Protocol command IDs and input/command report helpers (NS-LIB-HID).
 *
 * Copyright (c) 2026 Mitchell Cairns, Hand Held Legend, LLC.
 *
 * Licensed under the Creative Commons Attribution-NonCommercial 4.0 International License
 * (CC BY-NC 4.0). Non-commercial use with attribution; commercial licensing from Hand Held Legend.
 * Full terms: https://creativecommons.org/licenses/by-nc/4.0/legalcode — see LICENSING.md in this folder
 *
 * SPDX-License-Identifier: CC-BY-NC-4.0
 */

#ifndef NS_LIB_PROTOCOL_H
#define NS_LIB_PROTOCOL_H

#include <stdint.h>

#include "ns_lib_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_LIB_PROTOCOL_OUT_ID_RUMBLE_CMD 0x01
#define NS_LIB_PROTOCOL_OUT_ID_INFO 0x80
#define NS_LIB_PROTOCOL_OUT_ID_RUMBLE 0x10

#define NS_LIB_PROTOCOL_GET_STATE 0x00
#define NS_LIB_PROTOCOL_SET_PAIRING 0x01
#define NS_LIB_PROTOCOL_GET_DEVICEINFO 0x02
#define NS_LIB_PROTOCOL_SET_INPUTMODE 0x03
#define NS_LIB_PROTOCOL_GET_TRIGGERET 0x04
#define NS_LIB_PROTOCOL_GET_PAGELIST 0x05
#define NS_LIB_PROTOCOL_SET_HCI 0x06
#define NS_LIB_PROTOCOL_SET_SHIPMODE 0x08
#define NS_LIB_PROTOCOL_GET_SPI 0x10
#define NS_LIB_PROTOCOL_SET_SPI 0x11
#define NS_LIB_PROTOCOL_SET_NFC 0x21
#define NS_LIB_PROTOCOL_SET_NFC_STATE 0x22
#define NS_LIB_PROTOCOL_ENABLE_IMU 0x40
#define NS_LIB_PROTOCOL_SET_IMUSENS 0x41
#define NS_LIB_PROTOCOL_ENABLE_VIBRATE 0x48
#define NS_LIB_PROTOCOL_SET_PLAYER 0x30
#define NS_LIB_PROTOCOL_GET_PLAYER 0x31
#define NS_LIB_PROTOCOL_33 0x33

bool ns_protocol_generate_inputreport(uint8_t out[64]);

void ns_protocol_process_outputreport(const uint8_t *in, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_PROTOCOL_H */
