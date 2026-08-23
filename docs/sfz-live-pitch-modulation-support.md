# SFZ live pitch modulation support

Status: implemented in Phase 3.1.4.

Practical Sampler imports and evaluates the following SFZ tuning opcodes:

- `tune` for the static zone tuning offset, in cents.
- `tune_onccN` for controller-driven tuning, in cents.
- `tune_curveccN` for the optional native 128-point controller curve.

The importer resolves inherited opcodes and stores the modulation as prepared,
allocation-free metadata: controller number, signed amount in cents, curve
index, and a normalized 128-point lookup table. Curve references are validated
during import. Malformed or unsupported tuning modulation produces an import
finding instead of being silently discarded.

The same metadata is serialized into project and published instrument
manifests, then validated and restored when those manifests are loaded. This
keeps published playback behavior aligned with the authoring and preview paths.

At note-on, the current controller value is converted through the prepared
curve and added to the static `tune` value. A controller change for an active
voice updates its target sample increment without retriggering the sample,
resetting the position, changing the selected zone, or resetting envelope and
release state. The increment is smoothed over 32 rendered frames. Controller
events are processed at their sample offsets by the voice pool, so a held note
changes pitch at the event position rather than at the next audio block.

The existing linear sample interpolation remains the resampling method. No
external pitch-shifting library is required for this controller modulation
path. Higher-quality interpolation remains a separate, measurement-driven
follow-up if the supported tuning range exposes audible aliasing or transient
degradation.

Legacy projects without `tune_oncc` retain an inactive tuning modulation object
and preserve their existing static tuning behavior. The current runtime model
supports one prepared tuning modulation descriptor per route, matching the
existing controller-modulation representation.
