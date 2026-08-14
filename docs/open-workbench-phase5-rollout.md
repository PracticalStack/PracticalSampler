# Open Workbench Phase 5 rollout record

Date: August 14, 2026  
Status: implementation and automated qualification complete

## Delivered

- Added `drs_open_workbench_phase5_tests` and the aggregate `drs_open_workbench_qualification` build target.
- Added `tools/qualify-open-workbench-phase5.ps1` for Debug or Release builds, with optional VST3, responsiveness, and real large-instrument gates.
- Added deterministic coverage for 0, 1, 24, 642, and 1,000 zones; degenerate and overlapping geometry; discontiguous selection; viewport resize; every workbench state and tab; compact/expanded shells; and 100%, 125%, 150%, and 200% render scales.
- Added keyboard panning and visible focus to the minimap and completed accessible names, help text, and focus order for the map toolbar, minimap, splitter, and workbench controls.
- Removed the remaining Drawer-era identifiers from active authoring code, tests, and developer documentation. The supported internal term and component-ID prefix is `Workbench` / `authoringWorkbench`.
- Added the manual demonstration script and the semantic-token/render-policy extension guide.

## Qualification results

Qualification machine: Windows x64, Visual Studio 2022 17.14.34, local Accurate Salamander corpus.

| Gate | Result | Evidence |
|---|---:|---|
| Focused Debug mapping/waveform/macro/performance/authoring matrix | PASS | Eight behavior suites passed in 29.78 s; Phase 0 smoke passed in 5.52 s; Open Workbench Phases 0–5 passed individually. |
| Phase 5 Debug map profile | PASS | 1,000 zones; 16.0603 ms average paint; 0.002812 ms average input. |
| Phase 2 Release dense overview | PASS | 7.62073 ms average paint. |
| Phase 5 Release map profile | PASS | 1,000 zones; 5.51239 ms average paint; 0.000364 ms average input. |
| VST3 host-state qualification | PASS | Rebuilt VST3 bundle and host-state test passed in 2.38 s. |
| Performance-panel responsiveness | PASS | Debug test passed in 2.61 s. |
| Full Release responsiveness baseline | PASS | 1,700 zones and 637 sources; 180 s simulated playback completed in 53.93 s. Maximum concurrent UI dispatch was 3.480 ms and maximum package audio block was 0.232 ms. |
| Real large-instrument qualification | PASS | Accurate Salamander: 1,700 routes, 637 projected sources, 315.021 ms full-draft preparation, 10.44 MB resident heads, about 193 MB peak process working set. See `artifacts/open-workbench-phase5-large-instrument-release.md`. |
| Minimum shell and scale matrix | PASS (automated) | Standalone 900x700, 860x760, and 1120x800; plugin 760x620, 820x700, and 900x700; scale render passes at 100%, 125%, 150%, and 200%. |

The Debug responsiveness baseline also has a prior passing 1,700-zone report with a 3.493 ms maximum concurrent dispatch. During this qualification pass its synthetic audio worker did not finish three minutes of blocks before the Debug-only deadline (about 270 s); no UI latency assertion failed. The optimized build is the performance authority and passed the identical workload in 53.93 s.

## Test hardening found during rollout

- The Phase 0 shell smoke now closes a metadata-only performance-package workspace through the supported processor API before validating bundled authoring playback. It settles the message/audio activation boundary deterministically and verifies both standalone and plug-in audio.
- The waveform preview test now requests the current on-demand preview contract, waits for the matching active revision, clamps the audition note into the selected zone, and renders enough blocks to observe both fixtures. Recorded peaks were 0.478406 and 0.379977.
- Shell-heavy qualification executables use an 8 MiB test stack on MSVC. Production binaries and audio-thread behavior are unchanged.

## Issue triage

- Release-blocking: none found.
- Follow-up: execute the demonstration script on physical precision-trackpad hardware and visually inspect native 125%, 150%, and 200% OS scaling in the standalone app and a production VST3 host. Automated gesture, keyboard, shell-size, and scaled-render coverage is green.
- Production-UI backlog: final branding/iconography, optional dark theme, and a dedicated Perform-surface redesign remain outside this iteration.

## Rollout decision

Open Workbench is the default authoring surface in standalone and VST3 shells. There is no compatibility flag or alternate old-appearance path. The implementation is suitable for demonstrations with real instruments, subject only to the two recorded hardware/host visual follow-ups above.
