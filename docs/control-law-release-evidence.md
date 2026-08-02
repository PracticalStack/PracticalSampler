# Control-Law Release Evidence

Status: focused control-law proof complete; DAW-host sign-off remains a release-owner action.

The raw VST3 bundle builds successfully. The existing `drs.phase0.smoke` bootstrap-lifecycle
check is currently failing before its host scan because its standalone fixture does not observe an
initial default Performance activation. This is outside the control-law path and must be resolved
by the bootstrap-lifecycle owner before a full release sign-off.

## Compatibility promise

- Existing targets without `controlLaw` retain their saved `curve` and physical range. They are
  compiled as legacy behavior at publication and are never migrated on load, save, recall, or
  republish.
- Only the explicit **Upgrade mixer taper** authoring command changes an eligible legacy
  Gain/mix target to `drs.mixerGain.v1` with the frozen -96 to +6 dB range.
- New structured Gain/mix targets save `drs.mixerGain.v1`, version 1. Unknown, incomplete,
  future, or range-incompatible laws fail publication before activation and leave the last known
  good published table active.
- Host automation and saved host values are normalized macro coordinates. The callback, display,
  reset detent, and DSP physical value all derive from the same compiled law.

## Automated release matrix

| Proof | Automated coverage |
|---|---|
| All seven mixer anchors | `drs.control_law.release_proof` publishes each anchor, checks its Perform label and callback physical dB value, then measures `DspGain` output amplitude. |
| UI/runtime convergence | `drs.performance_mixer.ui` and `drs.phase2.performance_ui` check rendered controls, Unity/negative-infinity accessibility, published presentation, and value updates. |
| Project compatibility | `drs.control_law.project_schema` and `drs.sprint6.published_macro_binding` cover legacy omission, new law serialization, explicit migration undo/redo, malformed/incomplete law rejection, and future-version rejection. |
| 1/2/3/12 controls | `drs.control_law.release_proof` covers valid bounded publication; `drs.performance_mixer.s*` covers published topology, host-state recall, automation boundary, and republish churn. |
| Realtime handoff | `drs.sprint4_entry.realtime_guard` and the published-macro churn seams cover block-boundary activation, finite callback payloads, and audio-thread violation guards. |

The release build must pass `drs_all_tests` and the focused control-law matrix before manual
host sign-off.

## Manual host matrix

Follow [host-validation.md](host-validation.md) with the release VST3 bundle in the configured
REAPER scan path. For a migrated mixer project, write and read automation at 0.00, 0.05, 0.25,
0.50, 0.75, 0.85, and 1.00. Confirm the displayed values are respectively negative infinity,
-60, -30, -15, -6, 0, and +6 dB, the Unity detent returns to 0 dB, and reopening the project
preserves the same normalized host values. Attach the resulting REAPER track chunk and automation
capture to the release record.
