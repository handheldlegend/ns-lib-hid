# NS-LIB-HID implementation guide

This guide covers practical firmware integration for `NS-LIB-HID`: protocol
bring-up, descriptor registration, host OUT handling, haptics, and the weak
platform hooks your firmware is expected to own.

## 1) Integration model

`NS-LIB-HID` behaves like a small stateful protocol engine:

- initialize it once with `ns_api_init(...)`
- feed host OUT packets into `ns_api_output_tunnel(...)`
- call `ns_api_generate_inputreport(...)` on your input-report cadence
- override the weak `ns_api_hook_*` functions to provide real hardware state

The same core API is used for USB and Bluetooth transports. Your transport code
just moves bytes in and out; the library owns report formatting, subcommand
handling, SPI-style factory data, and rumble decode.

## 2) Build and include

From a parent CMake project:

```cmake
add_subdirectory(path/to/NS-LIB-HID)
target_link_libraries(your_firmware PRIVATE ns_lib_hid)
```

Include the public API:

```c
#include "ns_lib.h"
```

`ns_lib.h` also pulls in `ns_lib_hid.h`, which declares the USB / Bluetooth HID
descriptor helpers described below.

## 3) One-time initialization

Create and fill `ns_device_config_s`, then call `ns_api_init(&cfg)`.

Required fields:

- `type` must not be `NS_DEVTYPE_UNDEFINED`
- `transport` must not be `NS_TRANSPORT_UNDEFINED`
- `gyro_full_scale_dps` should match your IMU range if motion is used

Recommended fields:

- `device_mac`
- `host_mac`
- `colors`

Example:

```c
ns_device_config_s cfg = {0};

cfg.type = NS_DEVTYPE_PROCON;
cfg.transport = NS_TRANSPORT_USB;
cfg.gyro_full_scale_dps = 2000.0f;

// Fill cfg.device_mac / cfg.host_mac / cfg.colors as needed.

ns_config_status_t st = ns_api_init(&cfg);
if (st != NS_CONFIG_OK)
{
    // Handle configuration error.
}
```

`ns_api_init()` validates the configuration, computes derived gyro scaling,
initializes analog calibration defaults, and installs the built-in haptics
lookup tables/state.

## 4) HID and USB descriptor registration

When you register your USB device stack, or when you need to inspect what to
advertise over Bluetooth HID, use the helpers in `ns_lib_hid.h`.

- `ns_hid_get_device_descriptor()`
  Returns the standard 18-byte USB device descriptor.
- `ns_hid_get_device_name()`
  Returns the NUL-terminated product name string.
- `ns_hid_get_descriptor_params(...)`
  Returns pointers/lengths for the HID report descriptor and, when applicable,
  the USB configuration descriptor plus VID/PID values.

`ns_hid_get_descriptor_params(...)` only succeeds after transport has been
configured. In practice that means `ns_api_init(...)` should run before you ask
the library for descriptor pointers.

Conceptually:

- USB transport returns both HID and USB configuration descriptors
- Bluetooth transport returns the HID descriptor only
- unsupported or unset transport clears the outputs and returns `false`

Example:

```c
const ns_usb_device_descriptor_t *dev = ns_hid_get_device_descriptor();
const uint8_t *hid = NULL;
uint16_t hid_len = 0;
const uint8_t *cfg = NULL;
uint16_t cfg_len = 0;
uint16_t vid = 0;
uint16_t pid = 0;

if (!ns_hid_get_descriptor_params(&hid, &hid_len, &cfg, &cfg_len, &vid, &pid))
{
    // transport not configured or unsupported
    return;
}

// Register dev / hid / cfg with your USB or Bluetooth stack.
```

## 5) Host OUT path

Feed every received host OUT packet to:

```c
ns_api_output_tunnel(out_data, out_len);
```

Two things happen on that path:

- rumble bytes are decoded immediately if the packet carries haptics
- the protocol packet is queued so command handling can be serviced on the
  normal input-report cadence

The command FIFO depth is 8 packets. If your generate loop falls behind during
bursty host traffic, excess packets may be dropped.

Recommended policy:

- call `ns_api_output_tunnel(...)` as soon as OUT data arrives
- maintain an input-report cadence close to 8 ms when possible so host behavior
  stays predictable and queued commands are drained promptly

## 6) Generate path

Call `ns_api_generate_inputreport(...)` on your normal input-report tick with a
64-byte buffer. A cadence of about 8 ms is recommended.

```c
uint8_t tx[64] = {0};
bool ok = ns_api_generate_inputreport(tx);
```

When `ok` is `true`:

- `tx[0]` contains the report ID
- `tx[1..63]` contains the payload

If a queued command/info packet is pending, the output is a command reply.
Otherwise the library emits the normal `0x30` input report.

## 7) Haptics integration

Haptics are decoded on the OUT path, not on the generate path.

When a host packet contains rumble data:

1. the 4-byte rumble word is decoded immediately
2. the library updates its running raw haptics state
3. `ns_api_hook_set_haptic_packet_raw(...)` is called with the decoded packet

The raw packet is intentionally hardware-friendly:

- `packet->state` holds the current cumulative hi/lo band state
- `packet->samples[0..sample_count-1]` holds up to three sequential sub-samples
- each field is a lookup-table index, not a float amplitude or frequency

From there you have two common integration models.

### Raw-index / fixed-point path

This is the fastest option for hardware-specific playback engines.

At startup:

- call `ns_api_generate_fp_haptic_frequency_tables(...)`
- call `ns_api_generate_fp_amplitude_multiplier_table(...)`

At runtime:

- use the decoded indices from `ns_api_hook_set_haptic_packet_raw(...)`
- index directly into your precomputed fixed-point tables
- feed those results into your PWM, DDS, wavetable, or timer-based actuator path

### Processed-float path

This is useful for debug tooling, software synthesis, or an abstraction layer
that wants float reference values.

- inside `ns_api_hook_set_haptic_packet_raw(...)`, call
  `ns_api_convert_haptic_packet(...)`
- consume the returned amplitude/frequency values instead of the raw indices

Both paths can coexist. A common pattern is to use the raw path in production
and the float path for diagnostics or visualization.

## 8) Platform hooks to override

The library provides weak defaults so it can link out of the box, but a real
product should supply strong replacements for most of these:

- `ns_api_hook_get_time_ms(uint64_t *ms)`
  Monotonic millisecond clock used for report timing.
- `ns_api_hook_get_random_u8(void)`
  Non-cryptographic random byte source used in pairing-related flows.
- `ns_api_hook_get_input(ns_input_s *out)`
  Buttons and unpacked stick values; the library packs them into the report
  format internally.
- `ns_api_hook_get_powerstatus(ns_powerstatus_s *out)`
  Battery / charging / connection reporting.
- `ns_api_hook_set_led(int player_leds)`
  Apply host-selected player LED state.
- `ns_api_hook_set_power(uint8_t shutdown)`
  Respond to host shutdown requests.
- `ns_api_hook_set_usbpair(ns_usbpair_s pairing_data)`
  Persist pairing material if your product supports reconnect behavior.
- `ns_api_hook_set_imu_mode(ns_imu_mode_t imu_mode)`
  React to host IMU mode changes.
- `ns_api_hook_get_imu(ns_gyrodata_s *out)`
  Provide standard IMU samples.
- `ns_api_hook_get_quaternion(ns_quaternion_s *out)`
  Provide quaternion state for mode-2 motion reporting.
- `ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet)`
  Consume decoded rumble data.

## 9) Input representation and calibration

`ns_api_hook_get_input(...)` works with `ns_input_s`, which carries button
state plus unpacked stick positions. Firmware should populate those logical
values and let the library handle the Switch-specific stick packing internally
during report generation.

Calibration data for the SPI pages is initialized by
`ns_analog_calibration_init()` during `ns_api_init()`.

## 10) IMU and gyro scaling

Set `cfg.gyro_full_scale_dps` to match your sensor range, for example `2000`
for +-2000 dps operation.

The library derives `gyro_rad_per_lsb` from that setting during configuration
so report generation and motion helpers can consume a precomputed scale.

## 11) Minimal loop template

```c
void on_host_out_packet(const uint8_t *data, uint16_t len)
{
    ns_api_output_tunnel(data, len);
}

void protocol_tick(void)
{
    uint8_t report[64] = {0};

    if (ns_api_generate_inputreport(report))
    {
        transport_send(report, sizeof(report));
    }
}
```

## 12) Common pitfalls

- Calling the generate loop too slowly while the host sends bursts of commands.
- Forgetting to override the weak hooks, which leaves placeholder input,
  power, IMU, and haptics behavior in place.
- Requesting descriptors before `transport` has been configured.
- Treating decoded rumble values as physical units when they are still lookup
  indices.
- Using older symbol names from previous iterations of the library instead of
  the current `ns_api_*` / `ns_api_hook_*` surface.

## 13) Bring-up checklist

- Build links against `ns_lib_hid`.
- `ns_api_init()` returns `NS_CONFIG_OK`.
- Descriptor registration succeeds after transport is configured.
- Every host OUT packet is forwarded to `ns_api_output_tunnel()`.
- `ns_api_generate_inputreport()` is called with a 64-byte buffer at a cadence
  close to 8 ms.
- Your platform overrides provide real input, power, and persistence behavior.
- Haptics are either consumed directly from raw indices or mapped through one of
  the provided helper paths.
