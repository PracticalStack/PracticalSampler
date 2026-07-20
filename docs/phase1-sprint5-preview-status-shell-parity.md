# Mini Sprint 5.7 Preview Status And Shell Parity Contract

Completed July 19, 2026.

## Immutable publication

`Processor` publishes one immutable `AuthoringPreviewStatusSnapshot` through an atomic shared
publication. Shell and background readers copy only that publication; they do not inspect mutable
controller, preparation-worker, playback-context, activation, or voice state.

The snapshot carries typed preparation, activation, and presentation states together with request
scope and identity, current/requested/pending/active/failed/audible revisions, selected-zone
identity, requested and active prepared build IDs, snapshot/prepared digests, typed findings, and
last-known-good identity. Legacy strings remain presentation labels and guidance only.

## Responsiveness metrics

The controller records last and maximum durations for request to launch, launch to Ready, Ready to
activation, request to audible activation, and request to cancellation. The publication also
carries coalesced and canceled request counts plus current and maximum pending depth. Superseding
in-flight preparation records cancellation latency before advancing the request generation.
Metrics are observational and add no audio-thread work or locks.

## Creator presentation

Both shells use the shared authoring panel and the same status publication. Creator-facing labels
cover Preparing, Ready, Stale with last-known-good active, Failed with or without last-known-good,
Canceled, Superseded, and No Selection. Failure guidance uses the structured finding and selected
zone/source context.

The shared toolbar provides `Preview On`, which disables authoring audition and emits Preview-only
stop-all when turned off, and `Stop`, enabled only while Preview owns a note or has an active Preview
voice. The status surface includes audible latency and explicit last-known-good identity. Controls
have stable IDs, focus order, accessible titles, descriptions, and help text; compact bounds remain
inside the workspace. Neither control changes Performance activation or note ownership.

## Lifetime and parity

Standalone and VST3 editor surfaces receive the same typed lifecycle, revision, build, digest,
guidance, metrics, and control contract. Closing the plug-in editor does not own, reset, or mutate
the processor publication or active Preview model. Project Close remains the explicit operation
that clears Preview lifetime.

