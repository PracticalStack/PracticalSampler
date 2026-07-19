# Sprint 4.2 Deterministic Voice Kernel Contract

Status: authoritative Mini Sprint 4.2 DSP contract, completed July 19, 2026.

## Ownership and lifetime

`SamplerVoice` is shell-independent mutable DSP state in `drs_sampler_core`. It holds non-owning
pointers to one const `SamplerRenderModel`, route, and prepared sample. The Sprint 4.5
`SamplerPlaybackContext` retains the model/activation slot for at least the complete voice lifetime;
the voice never copies or releases shared ownership on audio. See the
[playback-context contract](phase1-sprint4-playback-context-contract.md).

`start()`, `render()`, and `reset()` are `noexcept`. They do not allocate, lock, resolve routes, open
files, decode samples, create strings, or destroy prepared resources.

## Frozen compatibility math

For effective MIDI note `n`, route root key `r`, source sample rate `sourceRate`, and output sample
rate `outputRate`:

```text
pitchRatio = 2 ^ ((n - r) / 12)
incrementFrames = pitchRatio * sourceRate / outputRate
```

For effective MIDI velocity `v` in 1 through 127 and route gain `gainDb`:

```text
baseGain = 0.25 * (v / 127) * 10 ^ (gainDb / 20)
```

The `0.25` headroom, velocity scale, dB conversion, double-precision cursor/increment, and
float-precision PCM accumulation preserve the existing authoring voice formula. Center-pan reference
vectors also match the existing processor renderer.

## Interpolation and frame traversal

- Cursor position begins at the prevalidated route start frame.
- The current frame is `floor(positionFrames)`.
- Linear interpolation uses the fractional cursor and the next frame.
- At the final source frame, the next-frame index clamps to that same final frame.
- The cursor advances after the frame is mixed.
- A voice becomes `finished` when its cursor reaches or passes the declared frame count.
- A one-frame source renders that frame exactly once. This closes the legacy `frameCount <= 1`
  silence edge while retaining all multi-frame center vectors.
- Equivalent sequential render ranges produce the same samples, cursor, and lifecycle as one
  contiguous range.

Loop wrapping and release envelopes are deliberately not part of the 4.2 kernel; Mini Sprint 4.4
owns those lifecycle additions.

## Channel, gain, and pan behavior

- Mono source PCM feeds both left and right output paths.
- Stereo source channels interpolate independently.
- Output is additive; the kernel never clears caller-owned buffers.
- Center-preserving linear balance is frozen as:

```text
leftPanGain  = pan > 0 ? 1 - pan : 1
rightPanGain = pan < 0 ? 1 + pan : 1
```

`pan` is prevalidated to `[-1, 1]`. Center remains unity in both channels and therefore preserves the
legacy center baseline. Hard left/right mute only the opposite channel. This activates authored pan
metadata that the legacy callback retained but did not apply.

The kernel writes output channel 0 and, when present, channel 1. Mono output therefore receives the
left path. Higher shell channel mapping remains outside the Sprint 4 stereo sampler contract.

## Numeric comparison policy

Approved deterministic vectors use an absolute float tolerance of `1e-6`. Pitch/gain setup uses
standard-library `pow` in double precision; small platform differences inside the tolerance are
accepted. Sample order, interpolation order, and per-voice accumulation order are fixed. Any change
outside tolerance requires an explicit baseline review rather than silently refreshing expected data.

## Invalid input behavior

Voice start returns false and leaves the voice idle for zero voice identity, invalid route index,
MIDI note outside 0–127, velocity outside 1–127, invalid output rate, or invalid retained sample
metadata. Render returns `accepted=false` without advancing the voice for missing output storage or
an output range outside the supplied non-owning buffer view.

## Deferred responsibilities

Mini Sprint 4.3 provides fixed-pool allocation, event scheduling, overflow, and stealing under
[its scheduling contract](phase1-sprint4-voice-pool-scheduler-contract.md). Mini Sprint 4.4 now
provides loops, release envelopes, and completion policy under
[its lifecycle contract](phase1-sprint4-voice-lifecycle-contract.md). Mini Sprint 4.6 owns the
production processor cutover; the legacy processor DSP remains active until A/B shell parity passes.
