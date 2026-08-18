# Waveform Workbench Phase 6 Evidence

## Outcome

Phase 6 is implemented. Practical Sampler now converts the supported SFZ region contract into its native project model, preserves it through save/reopen, DAW host state, playable-package compilation and package playback, and supports an independently versioned native loop crossfade. This remains a one-way conversion path; no SFZ writer or “export as SFZ” behavior was added.

## Interchange contract

- Effective SFZ inheritance is resolved before projection.
- `offset`, inclusive `end`, `loop_mode`, `loop_start`, and inclusive `loop_end` are converted to the native half-open playback-region contract.
- Embedded WAV loop metadata supplies a fallback when SFZ loop boundaries are omitted. Explicit SFZ values take precedence.
- Sample metadata is inspected once per canonical source path on the existing import worker path; the engine-side projection remains filesystem- and JUCE-independent through an injected resolver.
- Unsupported, malformed, or only partially representable region semantics are retained as explicit import findings/authoring notes.
- Runtime instrument schema 7 carries SFZ playback and loop fields. The obsolete playable-package loop rejection is removed only for packages that pass the new schema validation.

## Native loop crossfade

`loopCrossfadeFrames` is intentionally not presented as an SFZ opcode. It is a native extension with separate project/authoring/instrument schema increments (9/8/8), so portable SFZ semantics remain distinguishable from Practical Sampler enhancements.

The Waveform workspace exposes the value in frames or seconds, commits it through the existing single-edit Apply/Return interaction, includes it in undo/redo, and visualizes the head/tail overlap. Authoring clamps the crossfade to half the loop length and clears it when looping is disabled.

The voice kernel performs an allocation-free, equal-power cosine/sine blend only in the final authored crossfade window. Outside that window the render path is unchanged. A crossfading sample performs the existing tail interpolation plus one loop-head interpolation per channel. Resident and paged sources use the same kernel; missing tail or head pages follow bounded-silence policy while publishing imminent/look-ahead page intent and continuing musical time.

## Persistence and parity evidence

The deterministic suites cover:

- inherited SFZ values and explicit region overrides;
- inclusive-to-exclusive `end` and `loop_end` conversion;
- omitted values, WAV loop fallback, and explicit-SFZ-over-metadata precedence;
- unsupported loop-mode findings;
- native project save/reopen and host-state serialize/restore;
- playable-package build, open, preparation, render-route propagation, and production voice rendering;
- native crossfade authoring, accessibility, schema promotion, undo, and visualization state;
- equal-power vectors, short-loop clamping, pitched playback, partition invariance, voice reset/stealing, resident/paged equality, missing-page behavior, and offline-render parity.

The checked-in offline manifest now contains 22 reviewed scenarios, including `native-loop-crossfade`; its partition-invariance scenario also uses a native crossfade and is sample-identical for block sizes 32, 64, 127, 256, 512, and 1024.

## Validation run

- Debug repository-wide `drs_all_tests` build: passed (277 build actions).
- Debug focused suites: SFZ region contract, host-session state, package export lifecycle, authoring UI, voice kernel, fixed voice-pool scheduler, render model, prepared playback, publish preparation, and offline renderer passed.
- Release focused suites: SFZ region contract, host-session state, package export lifecycle, authoring UI, voice kernel, and offline renderer passed.
- `git diff --check`: passed; checkout EOL-policy warnings are informational.

## Deferred release qualification

Phase 6 bounds the crossfade cost structurally and proves deterministic behavior. The planned approximately 500 MB source/instrument workload, end-to-end CPU measurements, rapid editor churn, and source immutability audit remain Phase 7 release-activation work.

Automated listening parity remains a useful next iteration: render canonical note/event timelines from a reference SFZ player and Practical Sampler, align the WAV outputs, then compare sample error, null residual, spectral/energy envelopes, loop periodicity, onset timing, and perceptual thresholds. This supplements, rather than replaces, listening comparisons.
