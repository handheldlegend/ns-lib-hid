# NS-LIB-HID implementation guide

This guide covers practical firmware integration for `NS-LIB-HID` now that core protocol, haptics, SPI emulation, IMU paths, and **HID/USB descriptor blobs** are in place.

## 1) Integration model

`NS-LIB-HID` runs as a small stateful protocol engine:

- You initialize once with `ns_lib_init(...)`.
- You pass incoming host OUT packets into `ns_api_output_tunnel(...)`.
- The library queues those packets in a FIFO (depth 3).
- Your main loop periodically calls `ns_api_generate_inputreport(...)`.
- Each generate call:
  - processes at most one queued host command packet (if present), or
  - emits normal input report `0x30` if no command is pending.

This keeps host command handling deterministic and synchronized with your report cadence.

## 2) Build and include

From a parent CMake project:

```cmake
add_subdirectory(path/to/NS-LIB-HID)
target_link_libraries(your_firmware PRIVATE ns_lib_hid)
```

Include public API:

```c
#include "ns_lib.h"
```

`ns_lib.h` also pulls in **`ns_lib_hid.h`**, which declares USB/Bluetooth descriptor helpers (see §4).

## 3) One-time initialization

Create and fill `ns_device_config_s`, then call `ns_lib_init(&cfg)`.

Required fields:

- `type` (not `NS_DEVTYPE_UNDEFINED`)
- `transport` (not `NS_TRANSPORT_UNDEFINED`)
- optional but recommended: `device_mac`, `host_mac`, `colors`
- `gyro_full_scale_dps` (if <= 0, defaults to 2000 dps)

Example:

```c
ns_device_config_s cfg = {0};
cfg.type = NS_DEVTYPE_PROCON;
cfg.transport = NS_TRANSPORT_USB;
cfg.gyro_full_scale_dps = 2000.0f;
// fill cfg.device_mac / cfg.host_mac / cfg.colors as needed

ns_config_status_t st = ns_lib_init(&cfg);
if (st != NS_CONFIG_OK) {
    // handle configuration error
}
```

Notes:

- `ns_lib_init()` runs config validation, computes `gyro_rad_per_lsb`, initializes analog calibration defaults, and initializes haptics state.
- If you need to reconfigure at runtime, use `ns_device_config_set(...)`.
- **`transport` must be valid before descriptor queries succeed** (§4): `NS_TRANSPORT_UNDEFINED` and unsupported values such as `NS_TRANSPORT_UART` cause `ns_hid_get_descriptor_params` to return false.

## 4) HID and USB descriptor registration

When you register your USB device stack (or inspect what to advertise over Bluetooth HID), use the helpers in **`ns_lib_hid.h`**. They point at **ROM-constant** report and configuration blobs that match common Pro Controller–style layouts.

| Symbol | Role |
|--------|------|
| `ns_hid_get_device_descriptor()` | Standard **18-byte** USB device descriptor (VID/PID and string indices). |
| `ns_hid_get_device_name()` | NUL-terminated product name string for strings/USB product/BT friendly name flows. |
| `ns_hid_get_descriptor_params(...)` | Fills pointers/lengths for the **HID report descriptor** and optional **USB configuration descriptor**, plus **VID/PID**, based on the **configured transport**. |

### `ns_hid_get_descriptor_params` behavior

Signature (conceptually): output pointers for HID report bytes + length, USB configuration descriptor + length, VID, PID. **Returns a `bool`:**

- **`true`** — Transport is **`NS_TRANSPORT_USB`** or **`NS_TRANSPORT_BTC`**. Outputs are filled:
  - **USB:** HID report descriptor length **203** bytes; full-speed **configuration descriptor** length **64** bytes.
  - **Bluetooth:** HID report descriptor length **170** bytes; **no** USB configuration (configuration pointer is NULL, length **0**).
- **`false`** — Transport is still **`NS_TRANSPORT_UNDEFINED`**, or **`NS_TRANSPORT_UART`**, or any other unsupported value. **Do not use stale outputs:** every non-NULL output you passed in is cleared (NULL pointer and/or zero length; VID/PID set to 0).

**Call order:** Run **`ns_lib_init`** (or at least **`ns_device_config_set`** with a valid **`transport`**) **before** `ns_hid_get_descriptor_params`, or the call will fail until configuration is present.

Example (USB stack wiring — pseudocode):

```c
const ns_usb_device_descriptor_t *dev = ns_hid_get_device_descriptor();
const uint8_t *hid = NULL;
uint16_t hid_len = 0;
const uint8_t *cfg = NULL;
uint16_t cfg_len = 0;
uint16_t vid = 0, pid = 0;

if (!ns_hid_get_descriptor_params(&hid, &hid_len, &cfg, &cfg_len, &vid, &pid)) {
    // transport not set or unsupported — fix cfg.transport first
    return;
}
// Pass dev, hid/hid_len, cfg/cfg_len into your USB descriptor callbacks / registration.
```

For Bluetooth stacks, you typically consume **`hid` / `hid_len`** (and VID/PID if your stack exposes them); **`cfg`** remains unused (NULL / 0).

## 5) Host OUT path

Feed every received host OUT packet to:

```c
ns_api_output_tunnel(out_data, out_len);
```

Behavior:

- Data is forwarded into the protocol layer and may be **queued** in a FIFO (max 3 packets).
- Processing is deferred to generate-time.
- If the FIFO is full, **additional packets may be dropped**; avoid starving the generate loop under bursty host traffic.

Recommended firmware policy:

- Call `ns_api_output_tunnel(...)` as soon as OUT data arrives.
- Keep generate cadence high enough to drain the queue under bursty host traffic.

## 6) Generate path (main loop)

Call this on your normal report tick. The buffer must be **at least 64 bytes**; the library fills a full 64-byte Switch-style report (report ID at byte 0, payload follows).

```c
uint8_t tx[64] = {0};
ns_api_generate_inputreport(tx, sizeof(tx));
```

Output:

- `tx[0]` = report ID
- `tx[1..63]` = payload

If a command/info packet is queued, generated output is a command reply (`0x21` or `0x81`) with command effects applied in FIFO order.
If the queue is empty, generated output is normal report `0x30`.

## 7) Required callback overrides

The library provides weak defaults; real products should override these in platform code:

- `ns_get_inputdata_cb(ns_inputdata_s *out)`
  - Fill buttons and packed sticks.
- `ns_get_powerstatus_cb(ns_powerstatus_s *out)`
  - Set battery/charging/connection byte.
- `ns_get_imu_standard_cb(ns_gyrodata_s *out)` (if IMU standard mode enabled)
- `ns_get_imu_quaternion_cb(ns_quaternion_s *out)` (if IMU mode-2 enabled)

Useful host-effect callbacks:

- `ns_set_led_cb(int player_leds)`
- `ns_set_imumode_cb(ns_imu_mode_t mode)`
- `ns_set_haptic_indices_cb(...)`
- `ns_set_power_cb(uint8_t shutdown)`
- `ns_set_usbpair_cb(...)`

Platform utility hooks:

- `ns_get_time_ms(uint64_t *ms)`
- `ns_get_random_u8(void)`

## 8) Stick packing and calibration

For 12-bit Switch nibble layout use:

- `ns_analog_pack_xy12(x, y, out3)`

Byte mapping:

- `out3[0] = x[7:0]`
- `out3[1] = x[11:8] | y[3:0] << 4`
- `out3[2] = y[11:4]`

Calibration blob for SPI stick pages is initialized by `ns_analog_calibration_init()` during `ns_lib_init()`.

## 9) IMU and gyro scaling

Set `cfg.gyro_full_scale_dps` for your IMU range (for example 2000 for +-2000 dps).
`NS-LIB-HID` derives `gyro_rad_per_lsb` once during config set:

- `rad/s per LSB = (full_scale_dps / 32768) * (pi/180)`

Quaternion integration helpers consume this precomputed scalar.

## 10) Minimal loop template

```c
void on_host_out_packet(const uint8_t *data, uint16_t len)
{
    ns_api_output_tunnel((uint8_t *)data, len);
}

void protocol_tick(void)
{
    uint8_t report[64] = {0};
    ns_api_generate_inputreport(report, sizeof(report));
    transport_send(report, sizeof(report)); // USB/BT send path
}
```

## 11) Common pitfalls

- Calling generate too slowly while host sends multiple commands quickly (FIFO can overflow).
- Not overriding weak callbacks, causing all-zero input and placeholder state.
- Forgetting to set `type`/`transport` before `ns_lib_init()`.
- Calling **`ns_hid_get_descriptor_params`** before **`transport`** is configured, or using **`NS_TRANSPORT_UART`**, and ignoring the **`false`** return — you must not feed NULL/zero-length descriptor pointers into your stack.
- Passing a **buffer shorter than 64 bytes** to `ns_api_generate_inputreport` (the function will not fill a short buffer).
- Mixing API names from older trees; standardize on `NS-LIB-HID` symbols in your firmware (`ns_api_*`, `ns_lib_init`, descriptor helpers in `ns_lib_hid.h`).

## 12) Bring-up checklist

- Build links with `ns_lib_hid`.
- `ns_lib_init()` returns `NS_CONFIG_OK`.
- **USB:** After init, `ns_hid_get_descriptor_params` returns true and your stack receives non-NULL HID (and configuration) pointers with the expected lengths.
- **Bluetooth:** Same, with configuration descriptor absent (NULL / 0) and 170-byte HID report descriptor.
- OUT packets are passed into `ns_api_output_tunnel()`.
- Generate loop calls `ns_api_generate_inputreport()` with a **64-byte** buffer at a steady interval.
- Verified command bursts (up to 3) are handled in-order.
- Verified idle behavior emits `0x30` input reports.
