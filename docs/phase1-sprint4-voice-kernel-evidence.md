# Sprint 4.2 Voice Kernel Completion Evidence

Completed July 19, 2026.

## Implementation

- Added `SamplerVoice`, `SamplerVoiceStartRequest`, `SamplerVoiceRenderResult`, lifecycle primitives,
  and the center-preserving `computeSamplerPanGains()` function to `drs_sampler_core`.
- Voice state uses stable primitive identity, double cursor/increment, float gain/pan coefficients,
  and non-owning const model/route/sample references.
- Rendering is additive, bounded to the supplied non-owning output range, and performs no ownership
  exchange or topology repair.

## Approved reference and behavior matrix

`drs_sprint4_voice_kernel_tests` covers:

- the captured legacy center-pan stereo ramp vectors at unity pitch, 0 dB, and velocity 127;
- exact final-frame completion and no duplicate render after finish;
- octave up/down, fractional interpolation, and source/output sample-rate conversion;
- velocity 1 and 127 behavior plus -6.0206 dB gain;
- mono duplication, independent stereo channels, center/hard/half pan, and additive output;
- start offsets, output subranges, one-frame samples, and invalid output ranges;
- contiguous versus 3/4/5-frame partition equivalence at fractional pitch;
- invalid voice identity, route, note, velocity, and sample-rate admission;
- compile-time `noexcept` assertions for start and render.

## Automated integration

- Product target: `drs_sampler_core`
- Focused executable: `drs_sprint4_voice_kernel_tests`
- CTest name: `drs.sprint4.voice_kernel`
- Aggregate dependency: `drs_all_tests`

The aggregate build succeeded. The focused test passed, then the Sprint 4 entry/core matrix passed
7/7 in 15.09 seconds:

- all five `drs.sprint4_entry.*` tests;
- `drs.sprint4.render_model`;
- `drs.sprint4.voice_kernel`.

The production processor renderer is intentionally unchanged in this mini sprint. See
[the voice-kernel contract](phase1-sprint4-voice-kernel-contract.md) and
[the renderer boundary audit](phase1-sprint4-renderer-boundary-audit.md).
