# Mini Sprint 6.4 Bounded Publish Scheduling, Priority, And Cancellation

Completed July 20, 2026. This slice gives Preview and Performance preparation one fixed-capacity,
typed scheduler with explicit priority, fairness, cooperative cancellation, and measurable budgets.

## Scheduling contract

`PreparedPlaybackService` admits at most one pending candidate per lane and one in-flight build.
Same-lane admission replaces the older pending candidate. Performance has normal dispatch priority,
but a pending Preview is selected after at most three consecutive Performance dispatches. This keeps
an explicit Publish responsive while guaranteeing that sustained Publish traffic cannot starve the
newest Preview candidate.

The completion mailbox is also fixed-capacity. When it reaches four undrained results by default,
the worker applies backpressure and retains bounded newest queued work; it never evicts an
undelivered completion. Draining the mailbox wakes the worker. Thus request identity cannot be lost
and pending, running, and completed state are all constant-bounded.

## Supersession and cancellation

The Performance controller suppresses an exact live duplicate by the complete captured identity:
project generation, draft revision, authored-content digest, and macro-schema digest. A different
explicit request receives a monotonic request ID and cancellation generation and supersedes older
queued/preparing truth while the active last-known-good identity remains independent.

Each worker lane has its own atomic cancellation generation. A newer same-lane admission or an
explicit cancel invalidates an in-flight request without waiting for the worker mutex. The worker
checks that generation between source resolution, fingerprinting, decoding, zone binding, and final
digest construction. A cancellation rolls back cache entries created by that build and publishes
one typed canceled completion. Preview and Performance generations never invalidate one another.

## Typed diagnostics and budgets

Requests and results carry lane, priority, cancellation generation, enqueue order, queue wait,
request-to-ready duration, pending/running depth, and terminal disposition. The worker snapshot adds
dispatch/fairness counts, in-flight identity, completed-mailbox depth/backpressure, ownership bytes,
maximum observed latencies, configured limits, and violation counts. The same immutable scheduler
snapshot is exposed through facade diagnostics and performance reporting.

Default support budgets are:

| Measure | Bound |
|---|---:|
| Pending candidates | 2 |
| In-flight builds | 1 |
| Completed results | 4 |
| Consecutive Performance dispatches while Preview waits | 3 |
| Command to queued | 100 ms |
| Request to ready | 5 s |
| Retained prepared bytes | 512 MiB |
| Message-thread background service | 100 ms |

The facade measures the complete explicit command-to-queued and message-thread service intervals.
The Performance controller separately records request-to-ready latency, and the worker records queue
wait and request-to-ready values for each completion. Budget breaches remain observable diagnostics;
capacity budgets are enforced structurally by admission, coalescing, one in-flight job, and mailbox
backpressure.

## Deferred boundaries

- Mini Sprint 6.5 removes processor-owned activation eligibility/staging compatibility and proves
  every unsuccessful outcome preserves exact last-known-good.
- Mini Sprint 6.7 adds immutable stable-ID published macro value migration.
- Mini Sprint 6.8 replaces direct shell/UI facade calls and string-only presentation.
