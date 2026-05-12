# NS-LIB-HID

Portable C11 library for Switch-oriented gamepad behavior: protocol handling, haptics indices, SPI-style factory data, IMU helpers, and **USB / Bluetooth HID descriptor blobs** for Pro-style bring-up.

## Documentation

- **[docs/README.md](docs/README.md)** — index of guides  
- **[docs/implementation-guide.md](docs/implementation-guide.md)** — **start here** for integration: CMake, `ns_api_init`, host OUT → `ns_api_output_tunnel`, report generation, callbacks, haptics, and **descriptor registration**  
- **[docs/hd-rumble-implementation-guide.md](docs/hd-rumble-implementation-guide.md)** — HD rumble decode flow, raw/processed packets, and fixed-point table helpers  

Licensing: **[LICENSING.md](LICENSING.md)**.

## Quick start

```cmake
add_subdirectory(path/to/NS-LIB-HID)
target_link_libraries(your_firmware PRIVATE ns_lib_hid)
```

```c
#include "ns_lib.h"
```

See the implementation guide for the full init / OUT / generate loop and for wiring USB or Bluetooth stacks to the ROM descriptors.

## Haptics At A Glance

`NS-LIB-HID` decodes host rumble data as soon as an OUT report is ingested.
The decode result is exposed as a raw packet of lookup-table indices through
`ns_api_hook_set_haptic_packet_raw(...)`.

That packet can be consumed in two common ways:

- call `ns_api_convert_haptic_packet(...)` if you want float reference values
  for amplitude and frequency
- call `ns_api_generate_fp_haptic_frequency_tables(...)` and
  `ns_api_generate_fp_amplitude_multiplier_table(...)` during startup if your
  hardware playback path wants precomputed fixed-point tables for direct index
  lookup at runtime
