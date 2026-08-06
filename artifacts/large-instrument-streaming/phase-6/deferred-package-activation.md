# Phase 6 deferred package activation evidence

Date: 2026-08-05

## Production path

- `DeferredPackageSession` owns the validated v2 package path/TOC, package-backed data sources, cancellation generation, bounded head-preparation cursor, and immutable render-model factory.
- The typed lifecycle separates locator pending, metadata ready, source opening, head preparation, model construction, playable, pending activation, callback-confirmed active, degraded, failed, and cancelled states.
- `SamplerPlaybackContext::stageActivation` is the only activation seam. The prepared model pointer is consumed at an audio block boundary, and `observeAudioCutover` does not publish active readiness until the callback reports the expected revision/build identity.
- Replacement failures become degraded while the prior generation remains active. Existing generation-retirement ownership lets old voices keep their model/source/page leases through release.
- Host locator restore only queues a path and resolver. The resolver and package I/O run when the worker service advances the request.
- Plug-in and standalone status text/tooltips now call the same engine-owned readiness formatter. All deferred stages map to the shared `WorkspaceDocumentState` readiness vocabulary.

## Deterministic verification

Debug command: `build/vs2022-debug/tests/drs_package_v2_tests.exe`

Result: passed.

Latest lifecycle trace:

`metadataMicros=19 replacementCancel100Micros=278 locatorRequestMicros=6 activeGeneration=2`

The matrix verifies metadata-only readiness, automatic worker continuation, callback-boundary activation, old-generation voice retirement, degraded replacement, rapid cancel/replacement, deferred locator restore, v1 recovery guidance, and the shared UI status projection.

Production compilation:

- `DecentRhapsodyStudioApp` Debug: passed.
- `DecentRhapsodyStudioPlugin` Debug: passed.

## Remaining corpus gate

Automatic audible activation of the licensed Accurate Salamander v2 package cannot be qualified because the corpus is not present in the workspace or configured raw-sample location. The corpus gate remains open.
