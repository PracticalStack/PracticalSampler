# Sprint 6 Performance Publish And Activation GO Report

Decision date: July 20, 2026  
Decision: **GO - proceed to Sprint 7**

## Decision basis

Sprint 6 now provides one explicit typed Publish path from an exact captured authored revision to a
complete immutable Performance payload. Bounded background preparation, exact newest-wins
eligibility, controller authorization, block-boundary activation, off-audio retirement, immutable
old-generation voice ownership, and stable-ID macro migration preserve the active last-known-good
Performance through every unsuccessful path.

Mini Sprint 6.9 retired the remaining compatibility seams and added a registered mixed-lane closure
soak. The fresh Debug aggregate built successfully and the full matrix passed 70/70 in 124.38
seconds. A fresh Release VST3 was produced, and the representative smoke, Sprint 4 offline, Sprint 5
closure, Sprint 6 closure, realtime, benchmark, and load matrix passed 7/7.

The closure soak proves that Draft and Preview churn, selection, host automation, failure, and stale
work do not mutate Performance. Only the newest explicit successful Publish changes the active
payload. It also proves held-note generation ownership, coherent concurrent diagnostics, bounded
work/retention/retirement/voices, zero queue drops, zero callback overruns, and zero realtime
violations for the exercised support profile.

## Residual risks accepted

- ThreadSanitizer execution is a Linux CI gate; this Windows closure registers the new target but
  does not claim a local sanitizer run.
- The 8-second request-to-active and 250-millisecond message-service values are support ceilings for
  Debug and loaded environments, not product responsiveness aspirations. Sprint 8 retains final
  production profiling and creator validation.
- A `-j4` all-test run caused one inherited callback timing assertion under competing CPU-heavy
  tests. The gate passed alone and the full `-j2` matrix passed 70/70, so no test is waived.
- Streaming pressure, device lifecycle, host-session recall, moved-content recovery, and final
  fixture/reference removal remain Sprint 7-8 scope.

None of these residual risks blocks Sprint 7.

## Post-decision field correction

Later creator testing on July 20 found and corrected a Perform-keyboard audibility defect for narrow
authored zones. Published pitch and velocity modulation had been applied before route selection,
moving a physical gesture outside narrow key or velocity bounds. Routing now uses the physical
played note and physical gesture velocity first, retains effective velocity only as a compatibility
fallback, and applies modulation to the selected voice. The Perform panel's obsolete Preview
audition side path was removed at the same time.

An exact single-key/high-velocity published-project regression, a real JUCE keyboard mouse-click
render check in both shells, the shell routing audit, Sprint 6 closure soak, and realtime matrix pass
in rebuilt Release standalone and VST3 artifacts. The Perform surface additionally distinguishes an
inactive audio callback from publication readiness. This correction strengthens the Sprint 6
Performance-only routing invariant and does not change the GO decision.

The next creator retest identified an independent activation-selection failure: a reference/bootstrap
articulation id survived project replacement and could select zero routes from an otherwise valid
authored Performance payload. Activation now resolves an invalid selection against the immutable
authored articulation routes, preferring `default` and then the first deterministic non-empty route.
The exact render-model finding is preserved if construction still fails.

The Performance surface also replaces its clipped green combined status with a concise state chip;
Publish failure is red, and its complete actionable guidance and structured code are available beside
the keyboard and to assistive/tooltip readers. Debug and Release matrices covering mismatched authored
articulation activation, real keyboard audibility, failure presentation, shell parity, integration,
and realtime safety are green. This correction also leaves the GO decision unchanged.

## Sprint 7 handoff assumptions

Sprint 7 may rely on these invariants:

1. only `PerformancePublishCommandType::publishCurrentDraft` authorizes a Performance change;
2. request/result/activation identity and the active immutable payload are controller-owned;
3. failure, cancellation, supersession, staleness, and staging rejection preserve last-known-good;
4. activation and its macro callback view exchange atomically at an audio block boundary;
5. held and releasing voices retain their original immutable generation until cleanup;
6. Preview has independent controller, worker/event, activation, voice, and diagnostic ownership;
7. shells issue typed commands and consume one immutable typed presentation snapshot; and
8. all queues, histories, payload retention, retirement, and voice counts remain fixed-bounded.

Sprint 7 must preserve those invariants while adding streaming/page pressure, state save/recall,
moved-content recovery, device/sample-rate/block-size restart, suspend/resume, and host-session
lifecycle behavior.

## References

- [Mini Sprint 6.9 contract](phase1-sprint6-integration-hardening.md)
- [Mini Sprint 6.9 completion evidence](phase1-sprint6-integration-hardening-evidence.md)
- [Sprint 6 Publish contract](phase1-sprint6-publish-contract.md)
- [Mini Sprint 6.8 shell and status parity](phase1-sprint6-publish-shell-parity.md)
- [Sprint 5 GO report](phase1-sprint5-go-report-2026-07-19.md)
