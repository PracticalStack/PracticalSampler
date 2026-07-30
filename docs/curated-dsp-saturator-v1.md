# Curated Saturator v1

`drs.saturator` version 1 is a zero-latency, 1x stereo waveshaper. It deliberately has no
oversampling, look-ahead, dynamic allocation, or tail. Future transfer-function changes require
a new catalog algorithm version.

- `character`: `0` normalized tanh, `1` hard clip, `2` soft cubic.
- `driveDb`: 0–36 dB input drive.
- `tone`: a per-channel post-drive one-pole low-pass from 250 Hz to 12 kHz.
- `mix`: dry/wet balance; `outputDb`: -24–24 dB post-mix trim.
- Non-finite and subnormal samples are written as zero. State contains only the two tone filter
  samples and is reset deterministically on activation, reset, or sample-rate change.
