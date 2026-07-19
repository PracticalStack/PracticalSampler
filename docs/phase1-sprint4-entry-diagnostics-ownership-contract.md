# Sprint 4 Entry Gate Diagnostics Ownership Contract

Completed July 19, 2026. This contract is the EG3-T1 ownership map for processor diagnostics.
Only the message thread may assemble or publish `ProcessorRealtimeSafetySnapshot`; the audio callback
publishes a coherent numeric frame and never touches the authoring document or diagnostic strings.

## Publication path

1. The audio callback updates primitive atomic event counters and publishes `AudioDiagnosticsValues`
   through `AudioDiagnosticsPublication`. Its odd/even sequence brackets the frame, so the message
   thread retries instead of accepting a torn set of activation and voice values.
2. The message thread reads that frame, adds message-owned document and failure data, formats status
   strings, and atomically replaces a `shared_ptr<const ProcessorRealtimeSafetySnapshot>`.
3. UI and test callers atomically load the immutable pointer and receive a value copy. When an audio
   frame is newer, its primitive fields are overlaid on that private copy so callback counters remain
   immediate. Callers never read processor strings, vectors, activation objects, or authoring document
   state directly.

The handoff contains no mutex, wait, allocation, string operation, document traversal, or ownership
release on the audio side. Snapshot allocation and final destruction belong to non-audio readers.

## Ownership table

| Diagnostic fields | Writer/owner | Allowed readers | Synchronization and rule |
| --- | --- | --- | --- |
| `processBlockCount`, `lastProcessBlockMicros`, `maxProcessBlockMicros`, `overBudgetCallbackCount`, `callbackBudgetMicros` | Audio callback; preparation sets the initial budget | Message snapshot composer only | Independent atomics; maximum uses compare/exchange. |
| `samplePathResolutionsOnAudioThread`, `sampleDecodeEntriesOnAudioThread`, `referenceSampleLoadsOnAudioThread`, `authoringSampleLoadsOnAudioThread`, `activeVoiceCapacityGrowthCount`, `largeResourceReleasesOnAudioThread` | The instrumented operation increments its atomic; callback identity is thread-local | Message snapshot composer only | Primitive atomics. A concurrently running callback cannot misclassify message-thread work. |
| `preparedBlockSize`, `activeVoiceCapacityLimit`, primed capacity | Message/device preparation | Message snapshot composer only | Release/acquire atomics; callback does not mutate these configuration values. |
| `referenceSampleCountLoaded`, `referenceWarmupCount` | Message-thread reference warmup/cache service | Message snapshot composer only | Primitive atomics; no callback access to `loadedSamples`. |
| Performance and preview active voice counts/capacity | Audio callback | Message thread through the audio numeric frame | One odd/even-sequenced atomic frame; no reader inspects voice vectors. |
| Performance/preview activation counts and reclaimed/retired counts | Block-boundary callback or message-owned retirement, according to the event | Message snapshot composer only | Primitive event-counter atomics. |
| Active and pending performance/preview slot indices | Message thread stages pending/initial active; audio callback exchanges at block boundary | Message diagnostics derives identities only through atomic indices and per-slot diagnostic atomics | Acquire/release index exchange; activation objects remain private to their declared renderer/message owners. |
| Retired performance/preview queue indices | Audio callback produces; message thread consumes | Message diagnostics may read indices to calculate backlog | SPSC acquire/release indices. Diagnostics never scans or reads queue slot storage. |
| Deferred retirement backlog/bytes and queued retirement bytes | Message thread owns deferred values; producer/consumer adjust queued-byte atomics | Message snapshot composer and audio numeric publisher | Primitive atomics; payload objects and queue arrays are not diagnostic inputs. |
| Active, pending, and retired payload bytes | Message thread writes per-slot byte metadata before publication; audio/message publishers aggregate atomics | Immutable snapshot readers | Per-slot and queue-total atomics. No shared payload dereference is needed. |
| Active/pending Preview revision, published revision, and prepared build ID | Message thread writes per-slot identity before publishing its index | Audio numeric publisher and message snapshot composer | Per-slot atomics selected by acquire-loaded indices. |
| `currentAuthoringPreviewDraftRevision` | Message thread, from `AuthoringSession::getDocumentState()` | Message snapshot composer | Copied into an atomic before composition; callback never reads the document. |
| `failedAuthoringPreviewRevision`, `failedAuthoringPreviewState`, other load failure details | Message thread only | Message snapshot composer only | Plain message-owned values; strings are copied only while composing an immutable snapshot. |
| `authoringPreviewFailureState` | Message snapshot composer only | UI/tests through immutable snapshot copies | Failure text is never read from mutable processor storage by a UI caller. |
| `authoringPreviewRevisionState`, processor `state` | Message snapshot composer; a non-audio reader may refresh these in its private copy when overlaying a newer numeric frame | UI/tests through immutable snapshot copies | Formatted strings never enter the audio publication or shared mutable storage. |
| `publicationSequence` | Audio numeric publisher; message thread copies the accepted even value | UI/tests through immutable snapshot copies | Odd means write in progress; only a stable even sequence can be published. |
| `available` and the formatted `ProcessorRealtimeSafetySnapshot` base | Message thread | UI/tests | Atomic `shared_ptr<const ...>` replacement; each caller receives a stable value copy and may overlay a newer coherent primitive frame locally. |

## Worker boundary

Preparation workers do not write processor diagnostics. They publish normal EngineFacade completion
results; `serviceMessageThreadWork()` applies those results and updates processor diagnostics on the
message thread. This keeps worker lifetime and cancellation state out of the callback/UI boundary.

## String and failure-state rule

All shared diagnostic strings and failure details are message-owned. A non-audio reader may format state
labels in its own returned value copy after overlaying newer numeric facts. The callback must not construct,
clear, append, compare, or destroy a diagnostic string. A UI can retain its returned snapshot indefinitely
without observing later mutation.
