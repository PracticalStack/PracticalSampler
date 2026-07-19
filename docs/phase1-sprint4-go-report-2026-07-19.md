# Sprint 4 Shared Real-Time Sampler Core Report — July 19, 2026

## Decision

**GO — Sprint 4 is complete.** Mini Sprints 4.1 through 4.8 meet their exit criteria. The shared
sampler core is the only production renderer, Preview and Performance own independent mutable
playback state, deterministic baselines are reviewed, all callback guard and clean-load matrix
checks pass, and fresh Debug/Release validation is green.

Acceptance basis: implementation and evidence review by the Codex engineering agent; project-owner
acceptance through the Sprint 4 workflow.

## Validation gates

| Gate | Decision evidence |
| --- | --- |
| V1 — Boundary locked | `drs_sampler_core` consumes immutable prevalidated models and has no shell, document, filesystem, decode, or editor dependency. |
| V2 — Voice math trusted | Approved pitch, interpolation, gain, pan, channel, start-offset, final-frame, and split-block vectors pass. |
| V3 — Scheduling bounded | Each context owns 24 fixed voices and 128-event scratch; deterministic event ordering, overflow, stealing, and allocation guards pass. |
| V4 — Lifecycle trusted | Loop, release, completion, repeated note-off/reset, model lease, and partition-equivalence coverage passes. |
| V5 — Contexts isolated | Preview and Performance have separate voices, events, activation slots, retirement queues, and counters; the concurrency soak passes. |
| V6 — Shells cut over | Standalone, plug-in, and editor-closed paths delegate to the same core; processor legacy DSP/static scan is clean. |
| V7 — Render conformance | Twenty reviewed scenarios and the 5,000-frame 32/64/127/256/512/1024 partition matrix pass. |
| V8 — Sprint 4 GO | Fresh Debug 49/49 and supported Release 3/3 pass; architecture, contracts, CI, evidence, and roadmap are reconciled. |

## Final evidence

- Fresh Debug aggregate: passed; full CTest **49/49** in 157.16 seconds.
- Fresh Release: VST3 artifact built; smoke, offline conformance, and benchmark **3/3** in 8.03 seconds.
- EG4: 12 negative injections plus clean 48-voice/128-event loads at 44.1/48 kHz and
  32/64/128/256/512/1024 sample blocks.
- Concurrency: 5,032 callback blocks with simultaneous contexts, activation churn, immutable UI
  polling, release peaks, and off-audio retirement/reclamation.
- ThreadSanitizer: both concurrency targets are enforced by the checked-in Ubuntu Clang CI workflow;
  no local Windows TSan execution is claimed.
- Defects: one missing narrow-build VST3 dependency found and fixed; no open Sprint 4 blocker.

Detailed commands, timings, artifacts, benchmark measurements, and defect disposition are in
[the Mini Sprint 4.8 evidence](phase1-sprint4-integration-hardening-evidence.md).

## Residual risks and owners

| Residual risk | Owner | Disposition |
| --- | --- | --- |
| ThreadSanitizer executes in Linux CI, not on the Windows validation workstation. | CI maintainers | Accepted; deterministic Windows soak is green and CI builds both concurrency targets with TSan. |
| Supported callback budgets remain 44.1/48 kHz, 32–1024 samples, 128 events, and 24 voices per context. | Runtime performance owner | Accepted; expanding support requires a declared profile and new executable evidence. |
| Render timings use callback-local wall-clock microseconds and are diagnostic, not a cross-platform performance promise. | Diagnostics owner | Accepted; Sprint 8 owns production profiling across hosts and machines. |
| Streaming/page services remain Phase 1 compatibility infrastructure rather than Sprint 4 core behavior. | Sprint 7 owner | Planned; Sprint 7 integrates prepared stream resources and pressure recovery. |
| Immediate selected-zone Preview preparation remains a message-owned compatibility seam. | Sprint 5 owner | Planned; Sprint 5 replaces it with coalesced draft Preview control and visible readiness policy. |

None of these risks contradicts the Sprint 4 exit criteria.

## Sprint 5 handoff assumptions

1. Build Preview control around the existing isolated Preview context; do not add a second renderer.
2. Preserve selected-zone/draft activation as immutable model staging with block-boundary application.
3. Coalesce rapid edits outside audio and expose Preparing/Ready/Stale/Failed from immutable snapshots.
4. Keep Preview stop/reset, note ownership, steals, and drop diagnostics lane-local.
5. Retain the last successful Preview activation when a newer draft fails or is superseded.

## Sprint 6 handoff assumptions

1. Publish only a complete validated Performance payload; draft mutation must not touch active audio.
2. Reuse primitive block-boundary slot exchange and message-owned payload retirement.
3. Preserve old-model voice leases until natural or bounded release completes.
4. Bind automation/macro policy to explicit published identity without adding callback document access.
5. Surface activation success/failure/cancel/supersede and last-known-good identity through the
   existing sequenced diagnostics contract.

Sprint 5 may proceed. Sprint 6 may consume this renderer and activation lifetime contract without
reopening Sprint 4 architecture.

