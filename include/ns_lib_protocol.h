/**
 * @file ns_lib_protocol.h
 * @brief Host OUT-report ingestion and device IN-report generation for NS-LIB-HID (Switch-oriented HID).
 *
 * @par Model
 * - Call @ref ns_protocol_process_outputreport whenever the host sends an OUT report (USB interrupt or
 *   Bluetooth output). Haptics are decoded immediately; the packet may be queued (FIFO depth 3).
 * - Call @ref ns_protocol_generate_inputreport from your main loop / poll handler. It returns one
 *   64-byte IN buffer per call: either a deferred command/info reply, a one-shot USB init packet, or a
 *   standard 0x30 full controller report.
 *
 * @par Buffer layout (@p out[64])
 * - @c out[0] is always the HID **report ID** for the payload that follows in @c out[1..63].
 * - Internal helpers use @c NS_PROTOCOL_IN_IDX_* in @c ns_lib_protocol.c for timer, battery, ACK,
 *   subcommand echo, and payload regions — keep that .c file and this description in sync when changing
 *   layouts.
 *
 * @par Implementation review (ns_lib_protocol.c) — fixes to consider
 * The following issues are documented for follow-up in the .c implementation (not API changes):
 *
 * - **Uninitialized reply report IDs:** For queued @c NS_LIB_PROTOCOL_OUT_ID_RUMBLE_CMD /
 *   @c NS_LIB_PROTOCOL_OUT_ID_INFO handling, the generator does not always set @c out[0] to @c 0x21 or
 *   @c 0x81 before filling lower layers; add an explicit report ID (and preferably @c memset(out,0,64))
 *   for each reply path.
 * - **Input packing disabled:** @c _ns_protocol_set_inputdata leaves the @c memcpy into @c out commented
 *   out, so buttons and sticks may never reach the 0x30 report until restored.
 * - **GET_TRIGGERET helper:** @c _ns_protocol_set_subtriggertime assigns the same payload byte twice per
 *   loop iteration (lower half overwrites upper); likely needs separate column offsets (historically
 *   payload vs. payload+1 stride).
 * - **Pairing phase 2:** @c ns_set_usbpair_cb appears after a @c break in @c _ns_protocol_pairing_set case 2,
 *   so it is unreachable; reorder or fall through intentionally if pairing persistence is required.
 * - **Minimum @p len:** @c ns_protocol_process_outputreport should guard @p len against subcommand and
 *   SPI argument offsets (e.g. bytes 10+) before indexed reads.
 * - **Queue overflow:** When the FIFO is full the packet is dropped silently; callers may want a metric or
 *   hook.
 * - **Unknown queued OUT ID:** @c ns_protocol_generate_inputreport returns @c false for unrecognized
 *   @c pending.data[0] without defining @c out contents — callers should treat failure as “no valid IN
 *   packet” or zero the buffer.
 * - **Integration:** @c ns_api_generate_inputreport / @c ns_api_output_tunnel in @c ns_lib.c forward to
 *   these @c ns_protocol_* entry points with a 64-byte IN buffer and host OUT payloads respectively.
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

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name Host OUT report IDs (first byte of host → device packet) */
/** @{ */
#define NS_LIB_PROTOCOL_OUT_ID_RUMBLE_CMD 0x01 /**< Rumble + subcommand packet. */
#define NS_LIB_PROTOCOL_OUT_ID_INFO 0x80       /**< Vendor / info request. */
#define NS_LIB_PROTOCOL_OUT_ID_RUMBLE 0x10     /**< Rumble-only fragment (no subcommand). */
/** @} */

/** @name Subcommand IDs (byte index defined in ns_lib_protocol.c, typically offset 10) */
/** @{ */
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
/** @} */

/**
 * @brief Produce the next 64-byte host IN report into @p out.
 *
 * @param out 64-byte buffer. Byte @c out[0] is the HID report ID; bytes @c out[1..63] follow the layout
 *        built in @c ns_lib_protocol.c (0x30 full report, or 0x21 / 0x81 style replies when implemented).
 *
 * @return @c true if @p out was filled with a packet to transmit; @c false on generation failure or when
 *         an unrecognized queued OUT packet cannot be converted (see implementation notes above).
 *
 * @note On first call for @c NS_TRANSPORT_USB, may emit a one-shot 0x81 “init” style packet before normal
 *       0x30 reports.
 */
bool ns_protocol_generate_inputreport(uint8_t out[64]);

/**
 * @brief Ingest one host OUT report: decode haptics, optionally enqueue for deferred command handling.
 *
 * @param in Host payload; byte @c in[0] is the OUT report ID.
 * @param len Length of @p in in bytes (clamped internally to 64). Should be validated by the caller for
 *        protocol correctness; undersized buffers risk undefined behavior in the implementation until
 *        length guards are added.
 *
 * @note Safe to call from interrupt or USB callback if your platform rules allow; the FIFO is only three
 *       deep — burst traffic can drop packets.
 */
void ns_protocol_process_outputreport(const uint8_t *in, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_PROTOCOL_H */
