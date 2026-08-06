# Architecture Overview

This note captures the intended Phase 0 boundary lines for Decent Rhapsody Studio so future work grows around stable seams instead of drifting into an accidental HISE fork.

## Layer map

### `app/`

Product-owned JUCE shell code lives here.

- `drs_app_shared` owns the reusable status-panel UI that can surface runtime information in both standalone and plugin shells.
- `drs_app_shared` also owns shared authoring services for WAV import, project-source validation, and waveform-preview generation, so plugin and standalone shells stay thin clients over one asynchronous workflow.
- `drs_standalone_shell` owns the standalone-facing root component and window content.
- `drs_plugin_shell` owns the minimal plugin processor and editor shell.

This layer may depend on public headers from `engine_adapter/`, but it should not include HISE headers directly.

### `engine_adapter/`

This is the only product-owned layer allowed to mediate between the shell and HISE-facing concerns.

Current Phase 0 responsibilities:

- compile exact metadata about the vendored HISE snapshot into generated headers
- expose frontend-profile metadata through a lightweight linked bridge
- resolve the product-owned HISE content root under `content/hise_project/`
- present a stable `EngineFacade` surface to the shell

Early Phase 1 responsibilities now started in the same seam:

- define the first product-owned runtime manifest model
- load a versioned reference `.drinst` fixture into an in-memory runtime object graph
- validate the reference corpus paths and expose loader metrics to the shell

Sprint 4 adds a shell-free `drs_sampler_core` target in this layer. It owns immutable render-model
validation, sample interpolation, pitch/gain/pan math, fixed voice/event storage, loop/release
lifecycle, and one mutable `SamplerPlaybackContext` per lane. Preview and Performance share this
code but no voices, event scratch, activation slots, retirement queues, or counters.

Future HISE-backed runtime objects, preset loading orchestration, and processor-construction boundaries should be introduced here first.

### `content/`

This is product-owned authoring and runtime-facing data.

- HISE project assets live under `content/hise_project/`
- authored scripts, presets, sample maps, images, and XML backups belong here
- this tree is the product-owned source of truth for early authoring assets, not the vendored HISE tree

### `tests/`

This layer validates the product-owned seams without becoming a second application surface.

- `drs_phase0_smoke_tests` exercises `EngineFacade`
- the smoke harness validates that the authored HISE content root is present and populated
- the same executable instantiates the standalone and plugin shell components to catch immediate bootstrap failures
- Sprint 4 focused tests validate render-model rejection, voice vectors, scheduling, lifecycle,
  playback-context isolation, shell cutover, reviewed offline baselines, callback guards, and
  concurrent activation/diagnostic soak behavior

### `third_party/`

External source snapshots live here and must be treated as external even when vendored.

- `third_party/juce/`
- `third_party/hise/`

Local product work should prefer wrappers, generated config, and explicit integration seams over ad hoc edits inside vendored code.

## Dependency direction

The intended dependency flow is:

`app/` -> `engine_adapter/` -> vendored HISE metadata and content conventions

`tests/` -> `app/` and `engine_adapter/`

`content/` is data consumed by the adapter and, later, by runtime services. It should not depend on code layers.

## Guardrails

- Product-owned code outside `engine_adapter/` should not include HISE headers.
- Product-owned authoring assets should not be stored under `third_party/hise/`.
- Changes to vendored dependencies should be rare, explicit, and documented as external updates.
- New runtime integration work should first expose a stable adapter API before spreading through shell code.

## Current Phase 0 proof points

- Windows bootstrap builds the standalone shell, VST3 shell, HISE frontend compile probe, and smoke harness.
- The smoke harness runs through CTest and is wired into GitHub Actions.
- The shell can already surface adapter-driven status information without a full HISE runtime handshake.

## Sprint 4 renderer boundary

`PluginProcessor::processBlock()` is now an I/O adapter. It clears the host buffer, translates
bounded UI/host events, invokes the Performance and Preview contexts additively, and publishes
primitive diagnostics. It does not own voice DSP, route traversal, interpolation, envelopes,
looping, sample lookup, file access, decoding, or a reference-sample callback cache.

Immutable activation payloads are normalized into `SamplerRenderModel` instances off audio.
Message-owned code stages fixed activation slots; the callback exchanges only primitive slot
tokens at block boundaries. Voices retain raw const model views until completion, after which the
audio side returns a retirement token and message-owned service performs final payload release.

The sequenced diagnostic frame identifies each context and includes renderer timing, current and
peak active/releasing voices, steals, core/event-block drops, producer note-queue drops, activation
identity, payload bytes, and retirement state. UI readers consume immutable snapshots and never
inspect mutable renderer state.

## WAV import and startup boundary

WAV import now follows the same ownership pattern as the other product services:

- shell chooser callbacks submit immutable requests and return without copy, fingerprint, reader-open, or decode work;
- `WavImportService` owns staging, fingerprinting, inspection, cancellation, and terminal snapshot publication on a joined worker thread;
- `WavImportWorkflow` is completion-driven only and prepares apply/finalize/rollback commits from immutable terminal payloads instead of draining a synchronous authoring queue; and
- startup, project replace, close, migration, restore, and waveform selection all expose honest `idle` or `not-run` diagnostics until explicit import or preview work is requested.

## Large-instrument streaming boundary

`SampleDataSource` is the common resident/WAV/package source contract. Voices acquire bounded, lock-free frame views; workers parse WAV/RF64 ranges, authenticate package v2 records, and populate a byte-budgeted cache. Audio publishes fixed-capacity page intents and produces bounded silence on a miss while musical time advances.

Package v2 stores 64-bit TOC identities and independently sealed metadata, head, and page records. Streaming export writes one bounded record at a time to a stage file, verifies selected records, and atomically publishes. `DeferredPackageSession` advances metadata through heads/model/staging to callback-confirmed activation while old model/source/page ownership survives generation replacement.
