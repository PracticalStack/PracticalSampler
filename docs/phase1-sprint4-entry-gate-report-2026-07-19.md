# Sprint 4 Entry Gate Report — July 19, 2026

## Decision

**GO — Gate state: OPEN.** Sprint 4 shared-renderer extraction may begin. EG1 through EG5 meet the
entry conditions, the fresh Debug matrix is green, the supported Release smoke/benchmark set is
green, and no residual risk contradicts a gate requirement.

Reviewers recorded for this decision:

- implementation and evidence review: Codex engineering agent;
- acceptance authority: project owner through the Sprint 4 entry-gate workflow.

## Accepted evidence

| Gate | Accepted evidence |
| --- | --- |
| V1 — General authored preparation | New external WAV and FLAC sources pass Preview and Publish cold/warm preparation; container-free preparation, relink, same-path replacement, missing source, unsupported format, and cancellation pass. |
| V2 — Immutable payload ownership | Last-known-good Preview and Performance payloads survive queue drain and rejected newer work; bounded slot exchange, old-voice leases, close/restart, and off-audio reclamation pass. |
| V3 — Diagnostics concurrency | Concurrent callback, message service, activation churn, and immutable UI polling complete coherently; deterministic Windows coverage passes and Linux ThreadSanitizer CI is configured. |
| V4 — Real-time enforcement | Twelve isolated injected violations trip their intended counters; the declared 128-event, 48-voice-target clean render reports zero guard failures. |
| Shell parity | Standalone and editor-closed plug-in Preview/Publish paths report matching revisions, build IDs, digests, sample counts, retained bytes, audible output, and zero real-time failures. |

## Fresh-tree validation record

Repository validation used new, previously unused single-config Ninja directories.

| Validation | Result | Duration/evidence |
| --- | --- | --- |
| Debug configure | Passed | 42.6 s; 41 tests discovered, including five `drs.sprint4_entry.*` tests. |
| Debug `drs_all_tests` build | Passed | Initial clean aggregate build: 294 s. |
| Clean-tree defect review | Corrected | The first matrix passed 40/41; `drs.phase0.smoke` could not discover a VST3 because the aggregate omitted the plug-in artifact dependency. `DecentRhapsodyStudioPlugin` was added to `drs_all_tests`; the focused smoke then passed in 3.00 s. |
| Corrected Debug full CTest matrix | **Passed 41/41** | 134.36 s; zero failures. Entry tests: authored input 0.22 s, activation payload 2.78 s, diagnostics concurrency 0.55 s, real-time guard 6.20 s, shell parity 4.87 s. |
| Release configure | Passed | Fresh directory, 46.7 s wall time. |
| Release VST3 build | Passed | Fresh plug-in bundle produced and manifest generated. |
| Release supported set | **Passed 4/4** | 8.73 s: VST3 smoke 3.50 s, runtime baseline report 0.04 s, benchmark scene 4.71 s, baseline guard 0.24 s. |

Release artifacts also recorded a passing reference benchmark scene (ordinary playback, three-voice
moderate polyphony, preset reload, and load-profile switch) and a passing runtime baseline report.

## CTest and aggregate integration

The following targets are registered with CTest and built by `drs_all_tests`:

- `drs.sprint4_entry.authored_input`;
- `drs.sprint4_entry.activation_payload`;
- `drs.sprint4_entry.diagnostics_concurrency`;
- `drs.sprint4_entry.realtime_guard`;
- `drs.sprint4_entry.shell_parity`.

The aggregate also depends on `DecentRhapsodyStudioPlugin`, ensuring a clean-tree smoke run has the
VST3 bundle it is expected to discover.

## Residual risks and owners

| Residual risk | Owner | Disposition |
| --- | --- | --- |
| ThreadSanitizer runs in Linux CI rather than this Windows workstation. | CI maintainers | Accepted. Deterministic concurrent Windows coverage is green and the checked-in Clang/TSan workflow owns race-detector execution. |
| Budget enforcement covers only 44.1/48 kHz, 32–1024 samples, 128 events, and 24 voices per context. | Sprint 4 renderer owner | Accepted. Other host configurations remain explicitly unsupported by this gate and require a future budget declaration before support is claimed. |
| The Phase 1 reference playback route remains compatibility scaffolding while the shared renderer is extracted. | Sprint 4 renderer owner | Accepted. General authored preparation no longer depends on reference-stream membership; retiring the legacy rendering route is Sprint 4 scope. |

## Sprint 4 handoff assumptions

1. Consume only const activation payload and prepared-handle access from renderer code.
2. Keep Preview and Performance mutable voice/event state separate while sharing renderer logic.
3. Preserve block-boundary integer slot exchange, old-voice leases, and message-owned retirement.
4. Keep diagnostics publication non-blocking and retain all EG4 negative and clean-load regressions.
5. Treat the declared callback profile as the supported starting envelope; expand it only with new
   executable budget evidence.

The compact authoritative boundary is recorded in
[the Sprint 4 entry-gate contract](phase1-sprint4-entry-gate-contract.md).
