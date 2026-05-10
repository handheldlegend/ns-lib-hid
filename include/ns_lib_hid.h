/**
 * @file ns_lib_hid.h
 * @brief USB and Bluetooth HID report descriptors and device identity for Switch Pro-style emulation.
 *
 * ROM-backed descriptors match common Pro Controller layouts (USB full-speed composite + Bluetooth
 * gamepad). Call ns_hid_get_descriptor_params after ns_lib_init and ns_device_config_set so the correct
 * HID report descriptor is selected for the active transport (ns_device_config_s.transport).
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

#ifndef NS_LIB_HID_H
#define NS_LIB_HID_H

#include <stdbool.h>
#include <stdint.h>
#include "ns_lib_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Byte length of the USB HID report descriptor blob (see ns_lib_hid.c). */
#define NS_HID_USB_REPORT_DESCRIPTOR_LEN 203u

/** @brief Byte length of the Bluetooth HID report descriptor blob (see ns_lib_hid.c). */
#define NS_HID_BTC_REPORT_DESCRIPTOR_LEN 170u

/** @brief Byte length of the full-speed configuration descriptor (HID + vendor interfaces). */
#define NS_HID_USB_CONFIG_DESCRIPTOR_LEN 41u

/** @brief Byte length of the full-speed configuration descriptor (HID + vendor interfaces). */
#define NS_HID_USB_WEBUSB_CONFIG_DESCRIPTOR_LEN 64u

/** @brief Standard USB device descriptor size (USB 2.0). */
#define NS_USB_DEVICE_DESCRIPTOR_LEN 18u

/**
 * @brief Pointer to the constant USB device descriptor (VID/PID, strings indices, etc.).
 *
 * @return Non-NULL; do not modify the pointed-to struct.
 */
const ns_usb_device_descriptor_t *ns_hid_get_device_descriptor(void);

/**
 * @brief Human-readable product string used by the stack (e.g. USB string or BT name).
 *
 * @return NUL-terminated ASCII name; do not modify.
 */
const char *ns_hid_get_device_name(void);

/**
 * @brief Return descriptor blobs and IDs appropriate for the current transport.
 *
 * Succeeds only when ns_device_config_get reports NS_TRANSPORT_USB or NS_TRANSPORT_BTC (after
 * ns_device_config_set or ns_lib_init). For NS_TRANSPORT_BTC, configuration_descriptor is set to NULL and
 * configuration_descriptor_len to 0 (no USB configuration on that path). For NS_TRANSPORT_USB, the USB HID
 * report descriptor and composite configuration descriptor are returned.
 *
 * On failure (false return): NS_TRANSPORT_UNDEFINED (transport not configured yet), NS_TRANSPORT_UART, or any
 * other unsupported transport. Failed calls clear outputs: each non-NULL output pointer receives NULL or 0 as
 * described under the parameters below, so callers do not keep stale descriptor pointers.
 *
 * Any output pointer may be NULL if that value is not needed.
 *
 * @param[out] hid_report_descriptor      ROM HID report descriptor bytes, or NULL on failure.
 * @param[out] hid_report_descriptor_len  Length of that blob in bytes, or 0 on failure.
 * @param[out] configuration_descriptor   USB configuration descriptor; NULL for Bluetooth or on failure.
 * @param[out] configuration_descriptor_len Length in bytes; 0 when not applicable or on failure.
 * @param[out] vid                        USB vendor ID (same as device descriptor), or 0 on failure.
 * @param[out] pid                        USB product ID (same as device descriptor), or 0 on failure.
 * @return True if transport is USB or Bluetooth and outputs were filled; false otherwise.
 */
bool ns_hid_get_descriptor_params(const uint8_t **hid_report_descriptor, uint16_t *hid_report_descriptor_len,
                                  const uint8_t **configuration_descriptor, uint16_t *configuration_descriptor_len,
                                  uint16_t *vid, uint16_t *pid);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_HID_H */
