# Phase 1 Sprint 3 Preparation Boundary

This note completes `S3.1-T4` and `S3.1-T5` from the Sprint 3 companion plan on July 18, 2026.

It documents the single intended preparation boundary for the Draft-to-Playback pipeline before deeper Sprint 3 refactors move more code behind it.

## Boundary statement

There is exactly one intended product-owned preparation boundary for playback:

- the shell or facade may build immutable playback snapshots on the message side
- accepted Preview and Publish preparation work must cross into `PreparedPlaybackService`
- the audio callback may only consume already-installed immutable activations plus fixed realtime state

Everything else is either:

- authoring-only helper work
- bootstrap/reference fallback compatibility work
- or a temporary seam that Sprint 3 is explicitly trying to retire

## What is allowed

### Message-side responsibilities

The message side may:

- mutate the draft authoring document
- build immutable playback snapshots
- submit accepted Preview or Publish preparation requests
- service completed worker results back into contract state
- drain retired activation slots after block-boundary swaps

In current code, that seam is centered on:

- `EngineFacade::refreshPreviewToCurrentDraft()`
- `EngineFacade::publishCurrentDraft()`
- `EngineFacade::enqueuePreparedPlaybackBuild(...)`
- `PreparedPlaybackService::requestBuild(...)`
- `PreparedPlaybackService::enqueuePreviewBuild(...)`
- `PreparedPlaybackService::enqueuePublishBuild(...)`
- `PreparedPlaybackService::cancelQueuedPreviewBuilds(...)`
- `PreparedPlaybackService::cancelQueuedPublishBuilds(...)`

## Named service entry points

This section completes `S3.1-T5`.

Preview and Publish now have explicit named preparation-service entry points:

- Preview submission:
  - `PreparedPlaybackService::enqueuePreviewBuild(...)`
- Publish submission:
  - `PreparedPlaybackService::enqueuePublishBuild(...)`
- Preview cancellation:
  - `PreparedPlaybackService::cancelQueuedPreviewBuilds(...)`
- Publish cancellation:
  - `PreparedPlaybackService::cancelQueuedPublishBuilds(...)`

This matters because the earlier generic lane-based worker entry was flexible but too permissive as a boundary contract.

The new rule is simpler:

- if Preview needs playback preparation work, it enters through the Preview-named service call
- if Publish needs playback preparation work, it enters through the Publish-named service call
- new shell features should not bypass those calls by reaching directly into decode helpers or generic worker plumbing

### Worker-side responsibilities

The worker side may:

- turn immutable snapshot sample identities into immutable prepared sample and stream handles
- perform cache lookup, insertion, invalidation, and retirement bookkeeping
- produce prepared-build results for Preview and Publish lanes

In current code, that seam is centered on:

- `PreparedPlaybackService::runBackgroundWorker()`
- `PreparedPlaybackService::processQueuedJob(...)`
- `PreparedPlaybackService::prepare(...)`

### Audio-callback responsibilities

The audio callback may:

- observe active and pending activations
- swap activations at block boundaries
- render voices from already-prepared data
- update lock-free or callback-local counters

The audio callback must not:

- resolve manifest-relative or sample-relative filesystem paths
- load manifests or stream containers
- enter source-sample decode
- clear or destroy large prepared resources synchronously
- perform Preview or Publish preparation work directly

## What is not the boundary

The following seams are explicitly not the long-term playback preparation boundary, even though they currently touch sample files:

- `Processor::getAuthoringWaveformPreview()`
- `Processor::ensureSelectedAuthoringSampleLoaded(...)`
- `Processor::initializeReferencePlaybackAssets(...)`
- root-key restore helpers in `PluginEditor` and `MainComponent`
- bulk authoring import processing through `processNextAuthoringImportQueueItem(...)`

These are shell-side authoring or compatibility helpers. They must not become the way playback preparation is expanded for Preview or Publish.

## Practical rule for Sprint 3

When a new playback-preparation need appears, the default answer should be:

- build or extend it behind `PreparedPlaybackService`

The default answer should not be:

- add another direct `importSampleFile(...)` call in the shell
- resolve another sample path from `processBlock`
- or teach the audio callback to load or retire large resources itself

## Relationship to the audit

`phase1-sprint3-preparation-boundary-audit.md` records the current inventory, ownership tags, and callback counters.

This note is the policy layer on top of that audit:

- the audit says where ownership is blurred today
- this boundary note says which seam is allowed to survive and which seams are temporary debt
