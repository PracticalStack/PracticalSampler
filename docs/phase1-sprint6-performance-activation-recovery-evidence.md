# Mini Sprint 6.5 Completion Evidence

Date: July 20, 2026  
Decision: Pass

## Implemented evidence

- Added one immutable controller-authorized activation object with token, full request identity,
  snapshot/prepared build identity, five digests, retained bytes, and exact payload ownership.
- Moved authorization before render-model construction and activation-slot staging.
- Required exact token/build/digest/byte agreement when acknowledging the audio-boundary activation.
- Separated typed bootstrap initialization from explicit creator Publish behavior.
- Preserved active identity and payload through missing-source failure, staging rejection,
  cancellation, supersession, stale acknowledgement, and exact duplicate requests.
- Added message-owned pending-slot cancellation and bounded old-voice retirement/reclamation.
- Added last/maximum reclamation latency in rendered blocks to immutable context and processor
  diagnostics.
- Retired the processor-owned Performance eligibility/staging compatibility seam; five later-sprint
  expected-red seams remain.

## Dedicated matrix

`drs.sprint6.performance_activation_recovery` proves:

- authorization is visible before the audio boundary while the prior revision remains active;
- one block changes controller/context/diagnostics exactly once;
- an exact duplicate creates no new token, slot, or activation;
- canceled and invalid staging produce sample-exact last-known-good offline output;
- a newer activation retains an old held voice's payload bytes;
- note release produces a bounded retirement token and message-owned reclamation;
- reclamation backlog/bytes drain and block latency is measurable; and
- no large shared resource is finally released on the audio callback.

The controller matrix additionally covers malformed authorization, structured staging rejection,
failed identity independent from active identity, stale/repeated acknowledgement rejection,
preparation failure, cancellation, supersession, explicit repair, bounded history, and concurrent
immutable reads. Full-project conformance supplies missing-source and invalid-topology cases before
authorization.

## Validation results

| Check | Result |
| --- | --- |
| Debug `drs_all_tests` aggregate | Passed; 151 compile/link steps completed, including the new 6.5 executable. |
| CTest discovery | Passed; 65 tests discovered and the 6.5 matrix is registered as test 36. |
| Focused Sprint 4/5/6 regression set | Passed 9/9 in 47.61 seconds. |
| Sprint 6 controller, integration, scheduling, and 6.5 executables | Passed 4/4 directly after the aggregate relink. |
| Sprint 5 shell regressions reported by the Windows CTest runner | Passed 4/4 directly. |
| Concurrency soak | Passed three consecutive direct runs after requiring exactly one unchanged Performance activation during Draft macro churn. |
| Realtime guard and realtime safety | Both passed in isolated/direct runs; zero audio-thread resource-release violations. |
| Remaining registered matrix | Tests 41-65 passed 25/25 in 62.83 seconds. |
| Expected-red seam audit | Expected failure with exactly five later-sprint seams. |
| Diff hygiene | `git diff --check` passed. |

The Windows CTest runner intermittently reported access violations for unrelated JUCE shell
executables when run serially in a long segment, and timed out one realtime test. Every affected
binary passed when executed directly; the concurrency soak passed three consecutive direct runs.
This is retained as a runner anomaly rather than represented as a clean full-matrix CTest pass.

## Exit assessment

Mini Sprint 6.5 exit criteria and Gate A5 are met. A successful explicit Publish changes Performance
once at a block boundary; every unsuccessful path changes it zero times and preserves exact
last-known-good ownership. Mini Sprint 6.6 may proceed with voice-generation cutover policy.
