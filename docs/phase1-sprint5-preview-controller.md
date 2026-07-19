# Mini Sprint 5.2 Preview Controller Contract

Completed July 19, 2026.

## Ownership

`AuthoringPreviewController` is the product-owned, message-thread coordinator for Preview request
identity and lifecycle. `PluginProcessor` observes authoring state and adapts accepted controller
requests to the existing preparation and Preview playback boundaries; it no longer infers Preview
lifecycle from revision, selection, active-context, or prepared-build comparisons.

The controller does not decode samples, build render models, render audio, mutate the authoring
document, or publish Performance. The Preview playback context continues to own activation slots,
block-boundary installation, voices, and off-audio retirement.

## Request identity

Every eligible request has one immutable `AuthoringPreviewRequest` containing:

- a controller-issued request ID;
- cancellation generation;
- draft revision;
- selected-zone or current-draft scope;
- selected-zone identity, when required by scope;
- request reason; and
- deterministic request signature.

Selected-zone requests without a selection are rejected. An observation with the same revision,
scope, selection, and signature is a duplicate and creates no work. A different eligible request
supersedes the prior request and receives a new request ID.

## Lifecycle

Preparation follows the frozen Sprint 5.1 transition policy:

`Idle -> Queued -> Preparing -> Ready`

Queued or preparing work can be canceled or superseded. Preparing work can fail. A Ready request
can be superseded or report a staging failure without allowing a mismatched result to revive it.
Activation is independent and follows `NoActivation -> Pending -> Active`.

All transition methods reject an illegal or out-of-order transition. They do not silently repair
state. Controller counters record requests, supersession, rejected/accepted results, and completed
activations.

## Newest-result acceptance

A preparation result is accepted only when its full identity equals the current request and its
prepared build ID is nonzero. Request ID, cancellation generation, revision, scope, and selection
all participate in equality. A canceled, superseded, older, or otherwise mismatched completion
cannot become Ready, Pending, or Active.

The processor passes only the controller's current request into Preview staging, verifies the live
revision and selected-zone identity again, and then asks the controller to accept the immutable
prepared build before staging it through `SamplerPlaybackContext`.

## Incremental boundary

Mini Sprint 5.2 changes orchestration ownership, not the preparation implementation allocated to
Mini Sprint 5.4. The existing processor-local immediate payload construction and synchronous sample
warming fallback remain executable expected-red seams. They run downstream of typed request
observation and cannot decide controller result eligibility. Mini Sprint 5.4 moves them behind the
worker/preparation boundary and adds true current-draft model preparation.

Exact duplicate suppression is implemented here. Timed coalescing, bounded churn, and worker
cancellation are Mini Sprint 5.3 responsibilities. Audition-command unification is Mini Sprint 5.5,
and typed presentation state is Mini Sprint 5.7.

