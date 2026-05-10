# NS-LIB-HID

Portable C11 library for Switch-oriented gamepad behavior: protocol handling, haptics indices, SPI-style factory data, IMU helpers, and **USB / Bluetooth HID descriptor blobs** for Pro-style bring-up.

## Documentation

- **[docs/README.md](docs/README.md)** — index of guides  
- **[docs/implementation-guide.md](docs/implementation-guide.md)** — **start here** for integration: CMake, `ns_lib_init`, host OUT → `ns_api_output_tunnel`, report generation, callbacks, and **descriptor registration**  
- **[docs/hd-rumble-implementation-guide.md](docs/hd-rumble-implementation-guide.md)** — HD rumble tables and index decoding  

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
