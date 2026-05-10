# NS-LIB-HID documentation

Per-file licensing for this library is summarized in [**../LICENSING.md**](../LICENSING.md).

Markdown guides for integrating **NS-LIB-HID** live in this folder.

| Document | Purpose |
|----------|---------|
| [Implementation guide](./implementation-guide.md) | End-to-end integration: CMake, **`ns_lib_init`**, host OUT → **`ns_api_output_tunnel`**, **`ns_api_generate_inputreport`**, weak callbacks, **USB/Bluetooth HID descriptors** (`ns_hid_get_descriptor_params`, device descriptor, product string), and pitfalls |
| [HD rumble LUTs & index helpers](./hd-rumble-implementation-guide.md) | **`ns_haptics_build_raw_tables`**, turning frequency/amplitude **indices** into **float Hz / gain**, and how decoded packets map to those tables |
