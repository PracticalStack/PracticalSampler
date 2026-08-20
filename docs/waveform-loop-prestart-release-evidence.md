# Practical Sampler Pre-Start Loop Release Evidence

## Scope and decision

This release evidence covers the Practical Sampler repository only. The iteration allows an SFZ/native zone’s `loop_start` to precede its authored playback start while preserving the authored attack and the existing forward-loop modes.

Decision: the feature-specific release gate passes. The complete registered CTest aggregate remains an external qualification follow-up because it stalled in `drs.host_state.vst3_qualification` before reaching the remaining tests.

## Final accepted contract

For an enabled forward loop, all layers use the same native half-open invariant:

```text
loopStartFrame < loopEndFrame
loopStartFrame <= sampleStartFrame < loopEndFrame
loopEndFrame <= sampleEndFrame
```

When the persisted playback end is the historical zero sentinel, the physical source end supplies `sampleEndFrame` during validation. The loop head is retained independently from playback start. Note-on begins at `sampleStartFrame`; the first crossing of `loopEndFrame` wraps to `loopStartFrame`.

`loop_continuous` remains active through release. `loop_sustain` repeats while held, disables wrapping on note-off, and plays the remaining playback tail. Native loop crossfade remains bounded by half the loop length.

## Unsupported or rejected cases

- Missing loop boundaries when no SFZ, WAV, or known source metadata can resolve them: reported as unsupported.
- Collapsed or reversed loops: rejected.
- Loop end at or before playback start: rejected as non-enterable.
- Loop end beyond playback end or the physical source: rejected.
- Unsupported loop modes such as `ping_pong`, alternate/reverse loops, finite loop counts, and implementation-specific SFZ v2 behavior: reported as unsupported; never silently converted to a different mode.
- Native crossfade values larger than half the loop or enabled without a valid loop: rejected.

## Audit result

The active code, tests, fixtures, and Markdown policy were searched for the former full-containment rule and duplicated `loopStartFrame >= sampleStartFrame` lower-bound checks. Runtime, import, persistence, prepared-playback, render-model, publish, package, voice, and waveform-editor policy now use the relaxed enterable-loop invariant. The remaining “loop end inside playback” diagnostics are intentional upper-bound guards.

The waveform policy was updated so normalization, boundary editing, playback-region movement, and audition preserve an earlier loop head instead of silently clamping it to playback start.

## Feature-specific validation

The following focused gates passed in the Debug build:

- `drs.sfz_loop_prestart.baseline`
- `drs.sfz_region.contract`
- `drs.sprint31.sfz_projection`, including the real `DemoSFVInstruments/SoftStringSpurs/Soft String Spurs/Soft String Spurs.sfz` corpus
- `drs.sprint4.render_model`
- `drs.sprint4.voice_kernel`
- `drs.sprint4.voice_lifecycle`
- `drs.sprint4.offline_renderer`
- `drs.sprint6.performance_preparation`
- `drs.phase1.compile_path`
- `drs.phase1.playback_snapshot`
- `drs.package_v2.records`
- `drs.performance_package.export_lifecycle`
- `drs.phase1.performance_package_session`
- `drs.waveform_region.policy`

`git diff --check` passed; Git’s LF-to-CRLF notices are checkout policy warnings only.

Fresh `vs2022-debug` configuration passed. The aggregate `drs_all_tests` rebuild was started from that refreshed configuration but was stopped during the broad JUCE graph at 265/1,211 actions because the clean aggregate is substantially larger than the feature-specific gate; the affected product/test targets had already rebuilt and passed.

## Aggregate qualification follow-up

The registered 202-test aggregate passed the initial smoke and native-content tests, then produced no output for several minutes in test #3, `drs.host_state.vst3_qualification`. The run was stopped to avoid leaving an external host process unresolved. This does not invalidate the focused release gate above, but the external host qualification must be rerun before claiming a repository-wide aggregate pass.
