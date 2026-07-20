# Sprint 5 Authoring Preview Path GO Report

Decision date: July 19, 2026  
Decision: **GO — proceed to Sprint 6**

## Decision basis

Sprint 5 now provides a typed, bounded Preview controller and an independent Preview playback lane
for selected-zone and current-draft audition. Rapid authored edits coalesce behind a fixed deadline;
only the newest eligible immutable result activates; failure and staleness preserve last-known-good;
and both shells consume one immutable lifecycle, identity, metrics, and guidance snapshot.

Mini Sprint 5.8 removed the remaining temporary shell/processor duplication and added a registered
integration closure target. The final fresh Debug matrix passed 59/59 in 207.83 seconds. The fresh
supported Release matrix passed VST3 smoke, Sprint 4 offline conformance, Sprint 5 integration
hardening, and the benchmark scene 4/4 in 16.45 seconds.

The closure soak proves that unsaved Preview churn leaves the published Performance payload pointer,
revision, prepared build ID, prepared content digest, and offline-rendered samples unchanged. It also
proves bounded worker/controller work, bounded activation retention and retirement, zero queue drops,
zero callback overruns, and zero realtime violations for the exercised support profile.

## Residual risks accepted

- ThreadSanitizer execution is a Linux CI gate; this Windows closure only registers and builds the
  target locally.
- The 8-second request-to-audible value is a support ceiling for Debug/loaded environments, not a UX
  aspiration. Sprint 8 retains production profiling and creator-validation work.
- Streaming, device lifecycle, large-library recall, and final fixture/reference removal remain
  assigned to Sprints 7-8.
- Publish semantics are intentionally absent. No Sprint 5 Preview operation is authorization to
  mutate Performance.

None of these residual risks blocks Sprint 6.

## Sprint 6 handoff assumptions

Sprint 6 may reuse the immutable render model, fixed playback context, typed request/result identity,
newest-wins acceptance, block-boundary activation, off-audio retirement, and immutable status pattern.
It must add and validate, as explicit new product behavior:

1. an intentional Apply/Publish command;
2. a full-project Performance preparation request distinct from Preview;
3. last-known-good recovery for failed Performance publication;
4. held-note and releasing-voice cutover semantics; and
5. automation and macro state transfer across publication.

Performance remains read-only to authored Preview work until those Sprint 6 contracts pass.

## References

- [Mini Sprint 5.8 contract](phase1-sprint5-integration-hardening.md)
- [Mini Sprint 5.8 completion evidence](phase1-sprint5-integration-hardening-evidence.md)
- [Mini Sprint 5.7 status and shell parity](phase1-sprint5-preview-status-shell-parity.md)
- [Sprint 4 GO report](phase1-sprint4-go-report-2026-07-19.md)
