# Mini Sprint 5.3 Bounded Coalescing, Supersession, And Cancellation

Completed July 19, 2026.

## Product contract

Preview request observation remains message-thread owned. Distinct authoring candidates enter one
controller slot and use a 12 ms coalescing window with an absolute 40 ms launch deadline. Replacing
a queued candidate extends the quiet-window deadline but never the burst deadline. The controller
therefore retains at most one current candidate even when the authoring document produces hundreds
of revisions.

Project-open requests launch immediately. Direct audition can bypass the coalescing window only
when the exact requested content is already prepared. Otherwise direct audition remains subject to
the 40 ms maximum launch delay.

## Signatures and invalidation

`AuthoringPreviewInvalidationCategory` identifies mapping, gain, pan, root key, key bounds,
velocity range, sample start offset, loop, source assignment, selection, Preview scope, and general
authored topology changes. `buildAuthoringPreviewRequestSignature` combines scope, selected-zone
identity, category, and an authored-content fingerprint.

The processor fingerprint covers selected-zone source assignment/path, root/key/velocity bounds,
gain, pan, start offset, loop enable/range, scope, and selection. Current-draft scope hashes the full
serialized authored project. Mini Sprint 5.4 completed the corresponding preparation boundary.

## Supersession and cancellation

A newer queued candidate supersedes and coalesces the older candidate without launching it. A newer
candidate that replaces preparing work advances the cancellation generation and records a logical
cancellation. The processor also calls `EngineFacade::cancelPreviewPreparation`, which:

- cancels queued Preview worker jobs;
- closes the pending Preview contract request before consuming completions;
- drops Preview completion mappings, retaining no obsolete payload; and
- permits physically in-flight work to drain as an unowned result that cannot update Preview.

The facade now treats a same-revision pending or already-ready Preview refresh as idempotent. This
prevents direct callers from duplicating work already launched by the controller.

## Warm reuse

Accepted prepared results populate a bounded build-identity index. A newer request may discover a
warm result only when scope, selection, and request signature all match. The new request keeps its
own request ID, revision, and cancellation generation; only the immutable prepared build identity
is reusable. Different selections are never equivalent. Project reset clears warm records.

The index retains at most eight records by default and contains no prepared payload ownership.
Mini Sprint 5.4 materializes warm worker payloads through the scope-specific immutable model boundary.

## Bounded records and counters

Controller diagnostics expose requested, coalesced, launched, canceled, superseded, completed,
accepted, rejected, warm-reused, current pending depth, maximum pending depth, activation count,
configured timing, and retained completion-record count. Completion records contain only request
identity, terminal outcome, build ID, and acceptance; the default ring retains at most 32 records.

The prepared worker retains its existing two-job queue and one-job in-flight bounds. Controller
pending depth and worker queue metrics are tested independently.
