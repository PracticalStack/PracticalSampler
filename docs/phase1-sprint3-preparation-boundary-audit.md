# Phase 1 Sprint 3 Preparation Boundary Audit

This note completes `S3.1-T1` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: inventory the code paths that currently:

- resolve sample or manifest paths
- load manifest, container, or sample files
- decode WAV or FLAC content into memory
- retire or destroy prepared-playback resources

This is intentionally an inventory first. Thread ownership tagging and boundary decisions are left for `S3.1-T2` through `S3.1-T5`.

## Ownership tags

This section completes `S3.1-T2` by tagging each inventoried path with its current effective ownership:

- `message`: shell or facade caller-stack work that currently runs from UI, host callback setup, or explicit shell servicing outside the audio callback
- `worker`: dedicated background worker execution
- `audio`: realtime callback execution
- `mixed/unclear`: currently reachable from more than one thread class, or not yet fenced tightly enough to call safe

## Current ownership matrix

| Path | Current ownership | Evidence |
| --- | --- | --- |
| `engine_adapter/src/RuntimeLoader.cpp::resolveRelativePath`, `validateRequiredFile`, `validateRequiredDirectory`, `readTextFile` | `mixed/unclear` | Reference-manifest loading is used by shell diagnostics and status on the caller stack, but `Processor::ensureReferencePlaybackAssetsLoaded(true)` can also pull through `EngineFacade::loadPhase1ReferenceInstrument()` from `processBlock`. |
| `engine_adapter/src/RuntimeStream.cpp::resolveRelativePath`, `readTextFile`, `loadRuntimeStreamContainer` | `mixed/unclear` | Stream-container loading is used from ordinary shell reads and bootstrap, but the same reference-stream load can still be reached from `ensureReferencePlaybackAssetsLoaded(true)` on the audio path. |
| `app/src/plugin/PluginProcessor.cpp::resolveSamplePath` | `mixed/unclear` | Only used by reference-sample warmup, which currently runs both from non-realtime warmup and from the fallback audio-thread first-note path. |
| `engine_adapter/src/SampleImport.cpp::importSampleFile` | `mixed/unclear` | Used by waveform preview, root-key restore, authoring import, reference warmup, and selected-zone preview loading; reference warmup still reaches it from the audio callback when prewarm has not happened. |
| `engine_adapter/src/SampleImport.cpp::processNextAuthoringImportQueueItem` | `message` | Called from `PluginEditor`, `MainComponent`, and `Processor::initializeAuthoringImportMetrics()`, all on shell-owned caller stacks. |
| `app/src/plugin/PluginProcessor.cpp::getAuthoringWaveformPreview` | `message` | Only reached from editor and standalone UI preview providers. |
| `app/src/plugin/PluginProcessor.cpp::initializeReferencePlaybackAssets` | `mixed/unclear` | Explicit `invokedFromAudioThread` accounting and a live `ensureReferencePlaybackAssetsLoaded(true)` caller prove it still has both message-thread and audio-thread reachability. |
| `app/src/plugin/PluginProcessor.cpp::ensureSelectedAuthoringSampleLoaded` | `message` | Current production callers pass `false` only, through project replacement and preview-activation synchronization on shell-owned paths. |
| `app/src/plugin/PluginEditor.cpp::restoreSelectedZoneRootKey` | `message` | UI command handler only. |
| `app/src/standalone/MainComponent.cpp::restoreSelectedZoneRootKey` | `message` | UI command handler only. |
| `engine_adapter/src/EngineFacade.cpp::buildRejectedPreparedPlayback` | `message` | Runs synchronously on the same caller stack as rejected Preview or Publish requests. |
| `engine_adapter/src/EngineFacade.cpp::enqueuePreparedPlaybackBuild` | `message` | Request submission happens during Preview or Publish orchestration before worker handoff. |
| `engine_adapter/src/PreparedPlayback.cpp::requestBuild` | `message` | Assigned on the submitter stack before queue execution. |
| `engine_adapter/src/PreparedPlayback.cpp::prepare` | `mixed/unclear` | Accepted work runs on the prepared-playback worker, but rejected snapshot handling still calls `prepare(...)` synchronously on the facade caller stack. |
| `engine_adapter/src/PreparedPlayback.cpp::runBackgroundWorker` and `processQueuedJob` | `worker` | Dedicated `PreparedPlaybackService` background thread owns queued accepted Preview and Publish execution. |
| `engine_adapter/src/RuntimeStreamingService.cpp::runWorkerLoop` | `worker` | Dedicated streaming worker thread owns queued page-read completion. |
| `engine_adapter/src/RuntimeStreamingService` page-cache subsystem more broadly | `mixed/unclear` | Worker thread fills pages, but audio-facing voice advancement and lease queries observe and manipulate cache state from realtime-adjacent code. |
| `engine_adapter/src/PreparedPlayback.cpp::retireSupersededCacheEntries` | `worker` | Called from accepted prepared-build execution inside `prepare(...)`, which currently runs on the worker for real Preview and Publish jobs. |
| `engine_adapter/src/PreparedPlayback.cpp::retireStaleCacheEntries` | `mixed/unclear` | Explicit cleanup API exists, but current production ownership is not yet fixed; it is directly exercised by tests and can run on whichever caller drains retirement. |
| `app/src/plugin/PluginProcessor.cpp::drainRetiredAuthoringPreviewActivationSlots` and `releaseAuthoringPreviewActivationSlot` | `message` | Called during `serviceMessageThreadWork()` and shell-side synchronization, not from the audio callback. |
| `app/src/plugin/PluginProcessor.cpp::drainRetiredPerformanceActivationSlots` and `releasePerformanceActivationSlot` | `message` | Same pattern as authoring-preview retirement: enqueue at block boundary, release later on the shell/message side. |

## In-scope inventory

### 1. Manifest and container path resolution

- `engine_adapter/src/RuntimeLoader.cpp`
  - `resolveRelativePath(...)`
  - `validateRequiredFile(...)`
  - `validateRequiredDirectory(...)`
  - `readTextFile(...)`
  - used while loading runtime manifests and project-adjacent JSON assets
- `engine_adapter/src/RuntimeStream.cpp`
  - `resolveRelativePath(...)`
  - `readTextFile(...)`
  - `loadRuntimeStreamContainer(...)`
  - resolves stream-container-relative sample paths and recomputes source checksums against the current filesystem
- `app/src/plugin/PluginProcessor.cpp`
  - `resolveSamplePath(...)`
  - shell-local duplicate path resolution used when the processor warms reference playback samples from the checked-in reference stream container

Why this matters:

- path resolution is currently split across engine-side and shell-side helpers
- reference playback still has a processor-owned relative-path seam instead of consuming only product-owned prepared handles

### 2. Direct sample decode and in-memory sample loading

- `engine_adapter/src/SampleImport.cpp`
  - `importSampleFile(...)`
  - creates a `juce::AudioFormatReader`
  - accepts supported decoded formats from JUCE, then enforces the current Phase 1 WAV/FLAC policy
  - reads the full sample into a float buffer and copies normalized channel data into `ImportedSampleData`
- `engine_adapter/src/SampleImport.cpp`
  - `processNextAuthoringImportQueueItem(...)`
  - drives bulk authoring import through repeated calls to `importSampleFile(...)`
- `app/src/plugin/PluginProcessor.cpp`
  - `getAuthoringWaveformPreview()`
  - decodes the selected zone sample on demand when waveform preview cache is cold
- `app/src/plugin/PluginProcessor.cpp`
  - `initializeReferencePlaybackAssets(...)`
  - resolves each checked-in reference sample path and decodes it with `importSampleFile(...)`
  - fills `loadedSamples` for the current reference-backed performance path
- `app/src/plugin/PluginProcessor.cpp`
  - `ensureSelectedAuthoringSampleLoaded(...)`
  - decodes the selected authoring sample into `authoringLoadedSamples`
  - this is the current preview-activation seam that Sprint 3 needs to push behind worker-owned preparation
- `app/src/plugin/PluginEditor.cpp`
  - `restoreSelectedZoneRootKey()`
  - decodes the selected sample before calling root-key inference
- `app/src/standalone/MainComponent.cpp`
  - `restoreSelectedZoneRootKey()`
  - same decode-plus-inference path for the standalone shell

Why this matters:

- the sample decoder seam is still reused by UI helpers, waveform preview, reference warmup, and authoring preview activation
- prepared playback owns immutable sample and stream metadata, but the shell still owns several direct decoded-buffer lifetimes outside that service

### 3. Preview and publish preparation entry points

- `engine_adapter/src/EngineFacade.cpp`
  - `buildRejectedPreparedPlayback(...)`
  - calls `PreparedPlaybackService::requestBuild(...)` and `PreparedPlaybackService::prepare(...)` immediately for rejected or unavailable snapshot states
- `engine_adapter/src/EngineFacade.cpp`
  - `enqueuePreparedPlaybackBuild(...)`
  - submits accepted Preview and Publish requests into `PreparedPlaybackService`
- `engine_adapter/src/PreparedPlayback.cpp`
  - `requestBuild(...)`
  - validates immutable snapshot readiness and assigns prepared-build identity
- `engine_adapter/src/PreparedPlayback.cpp`
  - `prepare(...)`
  - converts immutable snapshot sample identities into immutable prepared sample and stream handles
  - performs cache lookups, cache insertion, and prepared-zone binding
  - does not decode source WAV or FLAC content today; it binds snapshot identities to the compiled stream container
- `engine_adapter/src/PreparedPlayback.cpp`
  - `runBackgroundWorker()`
  - executes queued Preview and Publish preparation jobs off the caller stack

Why this matters:

- this is already the strongest worker-owned preparation seam in the codebase
- Sprint 3 should expand this seam instead of growing more shell-local decode paths beside it

### 4. Stream-container and page-loading paths adjacent to preparation

- `engine_adapter/src/RuntimeStream.cpp`
  - `loadRuntimeStreamContainer(...)`
  - loads container metadata, page topology, and source checksum expectations from JSON
- `engine_adapter/src/RuntimeStreamingService.cpp`
  - worker loop and page-cache management
  - currently produces synthetic page data rather than decoding source sample payloads from disk

Why this matters:

- the stream service is already worker-backed, but it is still not the real source-audio decode boundary
- Sprint 3 needs to keep this distinction clear so sample decode is not confused with simulated streaming-page generation

### 5. Retirement and destruction seams

- `engine_adapter/src/PreparedPlayback.cpp`
  - `retireSupersededCacheEntries(...)`
  - removes stale prepared cache entries for an older cache key on the same sample source and moves them into `retiredCacheEntries`
- `engine_adapter/src/PreparedPlayback.cpp`
  - `retireStaleCacheEntries(...)`
  - drains the retired prepared-cache backlog and releases retained-byte accounting
- `app/src/plugin/PluginProcessor.cpp`
  - `drainRetiredAuthoringPreviewActivationSlots(...)`
  - `releaseAuthoringPreviewActivationSlot(...)`
  - retires old authoring-preview activation slots after block-boundary swap
- `app/src/plugin/PluginProcessor.cpp`
  - `drainRetiredPerformanceActivationSlots(...)`
  - `releasePerformanceActivationSlot(...)`
  - retires old performance activation slots after published activation swap

Why this matters:

- prepared-cache retirement is already explicit and measurable on the worker side
- shell-side activation retirement is separate from prepared-cache retirement, which means large lifetime transitions are not yet unified behind one product-owned cleanup policy

## Out-of-scope or adjacent paths

- `engine_adapter/src/RuntimeCompiler.cpp`
  - compiles imported metadata into runtime artifacts, but it does not perform live sample decode itself
  - it depends on `SampleImport` results that were already produced earlier
- `engine_adapter/src/EngineFacade.cpp`
  - several diagnostics and corruption-harness helpers read JSON text files
  - those reads are adjacent to the pipeline but are not the main prepared-sample ownership seam Sprint 3 is trying to move

## Current audit takeaway

The codebase already has one legitimate worker-owned preparation service: `PreparedPlaybackService`.

The remaining ownership leaks are not in immutable snapshot assembly anymore. They are in shell-side and authoring-side helpers that still call `importSampleFile(...)` directly:

- waveform preview
- selected-zone preview sample loading
- reference fallback sample warmup
- root-key restore helpers
- bulk authoring import processing

That means the next Sprint 3 steps should treat `PreparedPlaybackService` as the expansion point, while explicitly fencing off direct shell decode helpers so they stop looking like acceptable long-term runtime preparation seams.

## Ownership takeaway

The current map is clearer than it was at the start of Sprint 3:

- the strongest `worker` seams are `PreparedPlaybackService::runBackgroundWorker(...)`, `processQueuedJob(...)`, and `RuntimeStreamingService::runWorkerLoop(...)`
- the biggest `message` seams are UI helpers, root-key restore, shell import processing, and activation-slot retirement
- the most important `mixed/unclear` seams are still the ones touching reference warmup, manifest/container loading, and direct `importSampleFile(...)` decode

Those `mixed/unclear` tags are the Sprint 3 risk list. They identify exactly where the code still allows preparation ownership to blur across shell, worker, and realtime paths.

## Realtime counter coverage

This section completes `S3.1-T3`.

`plugin::ProcessorRealtimeSafetySnapshot` now tracks the forbidden callback-adjacent preparation seams more explicitly:

- `samplePathResolutionsOnAudioThread`
- `sampleDecodeEntriesOnAudioThread`
- `largeResourceReleasesOnAudioThread`
- existing sample-load and activation-retirement counters remain in place beside the new seam-specific counters

Current instrumentation points:

- `initializeReferencePlaybackAssets(...)`
  - increments path-resolution and decode-entry counters when reference warmup falls back into the audio callback
  - increments large-resource-release accounting if an existing reference cache would be cleared from the callback
- `ensureSelectedAuthoringSampleLoaded(...)`
  - increments decode-entry accounting if authoring sample loading ever reaches the callback in a future regression
- `releaseAuthoringPreviewActivationSlot(...)`
  - increments large-resource-release accounting if preview activation retirement ever occurs from inside `processBlock`
- `releasePerformanceActivationSlot(...)`
  - increments large-resource-release accounting if published activation retirement ever occurs from inside `processBlock`

Executable coverage now lives in `tests/src/Phase1RealtimeSafetyTests.cpp`:

- the normal primed playback path asserts that all new callback counters remain zero
- a forced fallback case clears the prewarmed reference cache and proves that callback-time path resolution and decode entry are both detected immediately
- activation-retirement coverage continues asserting that large activation cleanup drains on the message side, not in the callback

## Boundary documentation

This section completes `S3.1-T4`.

The single intended preparation boundary is now documented in:

- `docs/phase1-preparation-boundary-sprint3.md`

That note freezes the current rule:

- immutable snapshot construction may happen on the message side
- accepted Preview and Publish preparation must converge into `PreparedPlaybackService`
- the audio callback may only consume installed activations and must not become a preparation seam

Short code comments now reinforce that rule at the easiest violation points:

- `app/src/plugin/PluginProcessor.cpp::getAuthoringWaveformPreview()`
- `app/src/plugin/PluginProcessor.cpp::initializeReferencePlaybackAssets(...)`
- `app/src/plugin/PluginProcessor.cpp::ensureSelectedAuthoringSampleLoaded(...)`
- `engine_adapter/src/PreparedPlayback.cpp::prepare(...)`

## Named service entry points

This section completes `S3.1-T5`.

Preview and Publish are now expected to use explicit named preparation-service entry points instead of the older generic lane-plumbing surface:

- `PreparedPlaybackService::enqueuePreviewBuild(...)`
- `PreparedPlaybackService::enqueuePublishBuild(...)`
- `PreparedPlaybackService::cancelQueuedPreviewBuilds(...)`
- `PreparedPlaybackService::cancelQueuedPublishBuilds(...)`

`EngineFacade` now routes production Preview and Publish preparation through those named calls, and the focused prepared-worker regression uses the same API surface.

That gives Sprint 3 one concrete rule for new playback-preparation work:

- if the work belongs to Preview, it enters through the Preview-named service call
- if the work belongs to Publish, it enters through the Publish-named service call
