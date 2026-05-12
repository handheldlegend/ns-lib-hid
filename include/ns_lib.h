/**
 * @file ns_lib.h
 * @brief User-facing NS-LIB-HID API: configuration, host weak callbacks, platform hooks, and protocol
 *        entry points.
 */

#ifndef NS_LIB_API_H
#define NS_LIB_API_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ns_lib_types.h"
#include "ns_lib_analog.h"
#include "ns_lib_config.h"
#include "ns_lib_haptics.h"
#include "ns_lib_motion.h"
#include "ns_lib_spi.h"
#include "ns_lib_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- API Functions --- */

/**
 * @brief One-shot initialize: apply device config, then initialize runtime state.
 *
 * @param cfg Device configuration to apply.
 * @return `NS_CONFIG_OK` on success; validation error otherwise.
 */
ns_config_status_t ns_api_init(const ns_device_config_s *cfg);

/**
 * @brief Build one outgoing input report payload in-place.
 *
 * Writes report id to `data[0]` and payload to `data[1..]`.
 *
 * @param data[64] Output buffer of 64 bytes size.*/
bool ns_api_generate_inputreport(uint8_t data[64]);

/**
 * @brief Send raw output tunnel data to the USB/HID transport layer.
 *
 * @param data Pointer to the output packet payload.
 * @param len Length of the packet in bytes.
 */
void ns_api_output_tunnel(const uint8_t *data, uint16_t len);

/**
 * @brief Decoded HD-rumble index tuples callback (weak, user-overridable).
 * @param pairs Up to three decoded samples (float-only values).
 * @param pair_count Number of valid entries in @p pairs (0..3).
 */
void ns_api_convert_haptic_packet(ns_haptics_packet_raw_s *in, ns_haptics_packet_processed_s *out);

/* --- API Platform Hook Functions --- */

/**
 * @brief Monotonic time in milliseconds (weak).
 *
 * Firmware must set `*ms` based on the MCUs system time. Example: Time since boot. 
 *
 * @param[out] ms Elapsed milliseconds; ignored if NULL.
 */
void ns_api_hook_get_time_ms(uint64_t *ms);

/**
 * @brief Non-cryptographic random byte (weak).
 *
 * Used for Bluetooth Link Key placeholder generation during pairing. Replace with a hardware RNG or CSPRNG
 * if your product requires it.
 *
 * @return Uniform-ish byte in 0..255.
 */
uint8_t ns_api_hook_get_random_u8(void);

/**
 * @brief Decoded HD-rumble index tuples callback (weak, user-overridable).
 * @param pairs Up to three decoded samples (index-only values).
 * @param pair_count Number of valid entries in @p pairs (0..3).
 */
void ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet);

/**
 * @brief Host-set player LED callback (weak, user-overridable).
 *
 * @param player_leds Host player slot `1`–`8` from SET_PLAYER decoding, or `-1` when disconnected /
 *                    no valid assignment (unknown bitmask / cleared LEDs).
 */
void ns_api_hook_set_led(int player_leds);

/** @brief Host power-state request callback (weak, user-overridable). */
void ns_api_hook_set_power(uint8_t shutdown);

/** @brief USB pairing data callback (weak, user-overridable). */
void ns_api_hook_set_usbpair(ns_usbpair_s pairing_data);

/**
 * @brief Fill battery / USB status for input report byte @c 1 (weak, user-overridable).
 *
 * Set @ref ns_powerstatus_s bitfields (or assign @c out->val directly). Layout matches HOJA
 * `bat_status_u` / Switch Pro Controller wire format.
 *
 * @param out Output status; ignored if NULL.
 */
void ns_api_hook_get_powerstatus(ns_powerstatus_s *out);

/** @brief IMU Mode Setter.
 *
 * Set the IMU report mode.
 * 0 = disabled, 1 = raw IMU samples, 2 = quaternion.
 */
void ns_api_hook_set_imu_mode(ns_imu_mode_t imu_mode);

/**
 * @brief Fill IMU sample for standard mode reports (weak, user-overridable).
 * @param out Output gyro/accel sample.
 */
void ns_api_hook_get_imu(ns_gyrodata_s *out);

/**
 * @brief Fill quaternion state for IMU mode-2 reports (weak, user-overridable).
 */
void ns_api_hook_get_quaternion(ns_quaternion_s *out);

/**
 * @brief Fill packed input byte groups when needed (weak, user-overridable).
 * @param out Output button/stick raw bytes.
 */
void ns_api_hook_get_input(ns_input_s *out);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_API_H */
