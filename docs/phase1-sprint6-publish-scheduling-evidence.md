# Mini Sprint 6.4 Completion Evidence

Date: July 20, 2026  
Decision: Pass

## Implemented evidence

- Fixed pending work at two candidates, one per Preview/Performance lane, with one in-flight build.
- Prioritized newest explicit Performance work and forced Preview service after at most three
  consecutive Performance dispatches while a Preview waits.
- Retained completed results in a fixed mailbox and used backpressure instead of dropping completion
  identity.
- Preserved exact captured snapshots across edits made after enqueue.
- Suppressed exact live Publish duplicates and bounded controller pending truth and completion history.
- Added per-lane atomic cancellation generations and cooperative checks throughout source preparation,
  including rollback of partial cache ownership.
- Published typed lane, priority, cancellation, supersession, completion, depth, timing, fairness,
  ownership, backpressure, configured-budget, and violation diagnostics.
- Measured facade command-to-queued and message-thread service time plus controller and worker
  request-to-ready time.

## Validation scope

The dedicated `drs.sprint6.publish_scheduling` matrix covers a 1,000-request mixed-lane burst,
newest-only same-lane coalescing, strict Publish priority, bounded Preview fairness, immutable input
after enqueue, bounded completion backpressure, cooperative in-flight cancellation, partial-cache
rollback, forced time/memory budget breaches, 1,000 controller supersessions, 100 exact duplicates,
stale-result rejection, and newest-result acceptance. Inherited controller, facade, prepared-worker,
Preview coalescing, project reset, and full-project conformance tests cover close/cancel and the
existing lifecycle integration around the new scheduler.

## Validation results

| Validation | Result |
|---|---|
| Fully relinked Debug `drs_all_tests` build | Passed |
| Focused facade/worker/Preview/Publish matrix | **Passed 8/8** in 17.53 seconds |
| `drs.sprint6.publish_scheduling` consecutive direct repetitions | **Passed 10/10** |
| Registered tests 1-23 | Passed, including the 34.38-second strict realtime guard |
| Registered tests 24-40 | 16/17 passed in CTest; inherited Preview preparation had one runner timeout |
| Isolated inherited Preview preparation after that timeout | **Passed 10/10** in 46.6 seconds total |
| Registered tests 41-64 | **Passed 24/24** in 64.32 seconds |
| Direct expected-red audit | Required exit 1 with exactly **6** later-sprint seams |
| `git diff --check` | Passed |

All 64 registered targets therefore passed against the fully relinked binaries. A one-command CTest
attempt exhausted its outer six-minute command window after tests 1-9, and a resumed range later
recorded one 90-second runner timeout in the inherited `drs.sprint5.preview_preparation` target.
That unchanged target passed immediately in isolation and then passed ten consecutive repetitions;
no product limit or timeout was weakened, and no test was skipped or waived. A later focused CTest
handoff also exhausted its outer command window at the inherited Preview-coalescing target after
three preceding passes; the exact coalescing executable and the final 6.4 scheduling executable then
both passed immediately by direct execution.

## Exit assessment

Mini Sprint 6.4 exit criteria and Gate A4 are met. Pending, running, completed, history, and retained
work are constant-bounded; explicit Publish normally runs next; Preview has a finite fairness bound;
and only the newest exact eligible Publish identity can advance. Mini Sprint 6.5 may proceed with
atomic Performance activation and last-known-good recovery.
