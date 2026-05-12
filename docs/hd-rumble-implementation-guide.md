# HD rumble implementation guide

This guide explains how host rumble bytes become decoded haptics packets in
`NS-LIB-HID`, and how those packets can be consumed either as raw indices or
through the newer helper functions for float and fixed-point playback paths.

## 1) Decode flow

The normal integration path looks like this:

1. your USB or Bluetooth stack receives a host OUT report
2. firmware forwards it to `ns_api_output_tunnel(...)`
3. if the packet contains rumble data, the library decodes that data
   immediately
4. the decoded packet is delivered to `ns_api_hook_set_haptic_packet_raw(...)`

At the core of that path is:

- `ns_haptics_rumble_translate(const uint8_t *data)`

The decoder maintains a running hi/lo band state internally. Host packets are
delta-oriented and may encode up to three sequential sub-samples in one rumble
word, so the library reconstructs a full packet before calling your hook.

## 2) What the raw packet contains

`ns_haptics_packet_raw_s` is the firmware-facing raw representation.

- `sample_count`
  Number of valid sub-samples in the packet, from 0 to 3.
- `state`
  The most recent cumulative haptics state after decoding.
- `samples[3]`
  Up to three sequential sub-samples reconstructed from the current host word.

Each `ns_haptics_sample_raw_s` contains:

- `hi_amplitude_idx`
- `lo_amplitude_idx`
- `hi_frequency_idx`
- `lo_frequency_idx`

These are lookup-table indices, not direct physical units. That separation is
intentional: decoding stays cheap, and the application chooses the most useful
representation for its hardware.

## 3) Raw vs processed consumption models

There are two intended ways to consume the decoded packet.

### Raw-index path

Use the packet exactly as delivered by `ns_api_hook_set_haptic_packet_raw(...)`.

This path is best when:

- your haptics engine is fixed-point
- your actuator path is hardware-specific
- you want to avoid float math in the real-time playback loop

### Processed-float path

Call:

- `ns_api_convert_haptic_packet(ns_haptics_packet_raw_s *in, ns_haptics_packet_processed_s *out)`

This resolves the raw lookup indices into the library's reference amplitude and
frequency values, which is useful for:

- debug tooling
- visualization
- software synthesis
- actuator backends that prefer float inputs

## 4) Fixed-point table helpers

For hardware-oriented playback engines, the most useful haptics helpers are the
fixed-point table generators:

- `ns_api_generate_fp_haptic_frequency_tables(...)`
- `ns_api_generate_fp_amplitude_multiplier_table(...)`

The idea is to do the expensive math once at startup, then perform direct table
lookups during playback.

### Frequency tables

`ns_api_generate_fp_haptic_frequency_tables(shift, sine_table_width,
sample_rate_hz, hi_out, lo_out)` converts the library's hi/lo frequency
reference tables into fixed-point phase increments.

Typical interpretation:

- `shift`
  Q-format scale factor, such as `1u << 12` for Q12.
- `sine_table_width`
  Number of samples in your wavetable.
- `sample_rate_hz`
  PCM or update rate used by your playback engine.

Each output entry is effectively:

```c
phase_step = round((frequency_hz * sine_table_width / sample_rate_hz) * shift);
```

That makes the output a good fit for DDS or wavetable playback engines.

### Amplitude table

`ns_api_generate_fp_amplitude_multiplier_table(shift, out)` converts the
library's amplitude envelope table into fixed-point gain multipliers.

Typical interpretation:

- `shift`
  Q-format scale factor, again usually something like `1u << Q`
- `out[idx]`
  Fixed-point multiplier for one decoded amplitude row

This lets runtime code map a decoded amplitude index directly into a mixer, PWM
scaler, or DAC gain stage without recomputing the envelope curve.

## 5) Example integration pattern

Startup:

```c
enum { HAPTIC_Q = 12, SINE_LEN = 256, SAMPLE_RATE = 24000 };

static uint16_t hi_step[128];
static uint16_t lo_step[128];
static uint16_t amp_q[256];

void app_haptics_init(void)
{
    ns_api_generate_fp_haptic_frequency_tables(1u << HAPTIC_Q, SINE_LEN, SAMPLE_RATE, hi_step, lo_step);
    ns_api_generate_fp_amplitude_multiplier_table(1u << HAPTIC_Q, amp_q);
}
```

Runtime hook:

```c
void ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet)
{
    for (uint8_t i = 0; i < packet->sample_count; ++i)
    {
        const ns_haptics_sample_raw_s *s = &packet->samples[i];

        motor_set_hi(hi_step[s->hi_frequency_idx], amp_q[s->hi_amplitude_idx]);
        motor_set_lo(lo_step[s->lo_frequency_idx], amp_q[s->lo_amplitude_idx]);
    }
}
```

If you prefer float values instead, convert first:

```c
void ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet)
{
    ns_haptics_packet_processed_s processed = {0};
    ns_api_convert_haptic_packet(packet, &processed);

    // Consume processed.samples[0..processed.sample_count-1].
}
```

## 6) Timing note

One host rumble packet can represent up to three sub-samples. Treat them as a
short sequence in order:

- `samples[0]` first
- then `samples[1]`
- then `samples[2]`

Those sub-samples are the library's reconstructed steps for the current rumble
interval, not three unrelated independent commands.

## 7) Practical guidance

- If your firmware drives real hardware, prefer the raw-index path plus the
  fixed-point table helpers.
- If you are exploring behavior, logging rumble data, or validating a new
  actuator layer, the processed-float path is easier to inspect.
- `packet->state` is useful when your playback code wants the final cumulative
  result after the packet has been fully applied.
- `packet->samples[]` is useful when your playback engine wants to honor the
  packet's internal sub-steps.

## 8) Relevant files

- `include/ns_lib.h`
  Public API wrappers and weak hook declarations.
- `include/ns_lib_haptics.h`
  Haptics-facing public declarations and constants.
- `include/ns_lib_types.h`
  Raw and processed packet types.
- `ns_lib_haptics.c`
  Decode logic, reference tables, and fixed-point helper implementations.
