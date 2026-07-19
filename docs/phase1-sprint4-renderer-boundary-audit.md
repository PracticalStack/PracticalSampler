# Sprint 4.1 Renderer Boundary Audit

Completed July 19, 2026. This audit maps the sampler behavior still owned by
`app/src/plugin/PluginProcessor.*` to the product-owned Sprint 4 boundary. Mini Sprint 4.1 creates
the immutable input seam; later mini sprints move the DSP and mutable playback state through that
seam without widening the processor's responsibilities.

## Ownership map

| Current processor seam | Current responsibility | Sprint 4 destination | Migration slice |
| --- | --- | --- | --- |
| `RealtimeRenderRoute` / `resolveRealtimeRenderRoute()` | Reference-manifest zone/sample lookup | Prevalidated `SamplerRenderRoute` in the immutable render model | Model available in 4.1; callback lookup retired in 4.6 |
| `ActiveRenderVoice` | Cursor, pitch increment, gain, source identity, release state, activation lease | Core voice state plus context-owned lease | Completed in 4.2, 4.4, and 4.5 |
| `performanceActiveVoices` / `authoringPreviewActiveVoices` | Mutable dynamic voice pools | Separate fixed-capacity playback contexts | Core contexts completed in 4.3 and 4.5; shell cutover in 4.6 |
| `startVoiceForMidiMessage()` / `startAuthoringVoiceForMidiMessage()` | Route choice, prepared sample binding, pitch/gain setup, allocation/stealing | Shared renderer event application and voice allocator | 4.2, 4.3, and 4.6 |
| `releaseVoicesForMidiNote()` | Per-note release initiation | Core lifecycle command | 4.4 |
| `clearVoices()` | Context reset and lease cleanup | Context all-notes-off/emergency reset | Core behavior completed in 4.3 through 4.5; shell cutover in 4.6 |
| `renderBlockRange()` | Interpolation, loop traversal, release gain, channel mixing, completion | Shared voice kernel and renderer | 4.2 and 4.4 |
| `processBlock()` event splitting | Host MIDI offsets and inter-event render ranges | Shell translation plus bounded core event view/scheduler | View defined in 4.1; scheduler/cutover in 4.3/4.6 |
| `PerformanceRenderActivation` / `AuthoringPreviewRenderActivation` | Shell-specific prepared sample and snapshot projections | Immutable `SamplerRenderModel` retained by a playback-context activation | Model completed in 4.1; context integration completed in 4.5; shell cutover in 4.6 |
| activation slot arrays and SPSC retirement queues | Block-boundary index exchange and non-audio retirement | Context-owned primitive exchange with message-owned reclamation | Core contract completed in 4.5; processor replacement in 4.6 |
| `loadedSamples`, `authoringLoadedSamples`, `referenceManifest`, `referenceStream` | Legacy callback sample and route sources | Prepared handles and normalized routes retained by the render model | Callback dependency retired in 4.6 |
| performance/authoring SPSC note queues | Message/UI-to-audio bounded commands | Remain shell adapters feeding context-owned event scratch | 4.3 and 4.6 |
| primitive diagnostic atomics/sequenced snapshot | Callback facts and cross-thread inspection | Remain shell publication boundary; source counters move to core results | 4.8 reconciliation |

## Boundary created in 4.1

- `drs_sampler_core` is a separate static target with only product headers and C++17 runtime
  dependencies. It does not link JUCE, sample import, filesystem, worker, editor, or device code.
- `buildSamplerRenderModel()` is the single message/worker-owned construction seam.
- A successful model retains the complete `shared_ptr<const PlaybackActivationPayload>` and exposes
  only const identity, sample, and route views.
- Prepared PCM is shared through existing immutable handles; decoded channels are not copied.
- Model construction validates payload identity plus all renderer-relevant topology before a model is
  published. Findings allocate strings only on the non-audio construction path.
- `SamplerAudioBufferView`, `SamplerRenderEventView`, `SamplerRenderRequest`, and
  `SamplerRenderResult` establish non-owning callback vocabulary without implementing DSP early.

## Deliberately unchanged in 4.1

The production processor still renders through its legacy voice structs and functions. Moving DSP in
the same slice as defining its input contract would prevent isolated review of the boundary. Mini
Sprints 4.2 through 4.6 must migrate those responsibilities and delete the legacy implementation only
after focused and A/B parity evidence is green.

## Proof

`drs.sprint4.render_model` verifies valid Preview/Performance construction, const access, activation
and PCM lifetime, non-owning views, and negative identity/topology cases. The test is registered with
CTest and its target is included in `drs_all_tests`.
