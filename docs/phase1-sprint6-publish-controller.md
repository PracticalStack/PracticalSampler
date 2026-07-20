# Mini Sprint 6.2 Performance Publish Controller And Lifecycle

Completed July 19, 2026. This slice centralizes Publish request/result eligibility and immutable
lifecycle truth while deliberately retaining the Sprint 4 processor/context activation mechanism.

## Controller authority

`PerformancePublishController` is message-thread owned. It is the sole authority for:

- monotonic request ID and cancellation generation;
- captured project generation, draft revision, authored content digest, and macro schema digest;
- typed preparation and activation lifecycle;
- exact duplicate suppression and supersession;
- current-result eligibility and stale-result rejection;
- structured failed identity independent from active identity;
- accepted and active prepared build/content/schema identity;
- bounded completion history and lifecycle/latency counters; and
- immutable snapshot publication for concurrent readers.

Shells still call `EngineFacade::publishCurrentDraft()` for compatibility until Mini Sprint 6.8,
but the facade wrapper now captures the authored snapshot identity and delegates lifecycle authority
to the controller. It cannot independently decide that a result is Ready or Active.

## Request behavior

A request is accepted only with nonzero project generation and nonempty authored/macro schema
digests. An exact live request for the same captured project/revision/content/schema is suppressed
without allocating a new controller identity or queueing new worker work. A different explicit
request receives a newer request ID and cancellation generation. If older work is queued, preparing,
or pending activation, it is recorded as superseded; preparing work also requests cancellation.

A failed captured input may be explicitly retried. The retry receives a new request identity rather
than being mistaken for an exact live duplicate.

## Result and activation behavior

The facade stores controller identity beside each pending Performance worker completion. On
completion it constructs a typed `PerformancePublishResult`; the controller accepts it only when:

- the full identity equals the current request;
- both snapshot and prepared results cover the complete project and are activation eligible;
- the prepared build ID and content digest are present; and
- the prepared macro schema digest equals the captured schema digest.

Successful acceptance yields Ready. Processor staging uses the existing immutable render-model and
Performance activation slots, then reports Pending to the facade/controller. After the audio block
applies the exact revision/build, message-thread reconciliation reports Active. This slice does not
move model construction, eligibility, or controller mutation onto the callback.

Failure, cancellation, and supersession update requested/failed state without clearing the stored
active request/build/digests. Project close or project-identity replacement resets request and active
controller identities and advances generation so old completions cannot revive.

## Immutable publication

The controller maintains a message-owned working snapshot and atomically publishes a
`shared_ptr<const PerformancePublishControllerSnapshot>` after each accepted transition/counter
change. Readers receive a by-value copy of that immutable publication. They do not inspect mutable
controller or worker storage, and concurrent reads cannot observe half-written identity/digest state.

## Deferred boundaries

- Mini Sprint 6.3 owns full-project general-authored preparation completeness.
- Mini Sprint 6.4 owns final Publish/Preview priority, cooperative worker cancellation, and budgets.
- Mini Sprint 6.5 removes processor-owned activation eligibility/staging compatibility.
- Mini Sprint 6.7 adds immutable stable-ID published macro bindings.
- Mini Sprint 6.8 replaces direct shell/UI facade calls and string-only presentation.
