# Sprint 4 Entry Gate Real-Time Guard Contract

Completed July 19, 2026. EG4 extends the original realtime-safety counters into an executable,
test-scoped guard for every prohibited Sprint 4 callback operation.

## Audio-thread scope

`ScopedRealtimeAudioThread` installs the current processor's `RealtimeGuardState` in thread-local
storage for the duration of `processBlock()`. Guard entry points perform one lock-free atomic increment
and never allocate, throw, lock, wait, or format text. State is processor-owned, so concurrent processor
instances and focused negative cases do not contaminate one another.

The focused EG4 executable replaces global allocation/deallocation operators only in that test binary.
Production allocation behavior is unchanged. An allocation or deletion that occurs while the scoped
callback is active reports into the processor's guard before delegating to the normal allocator.

## Guard matrix

| Guard | Product/test entry | Snapshot evidence |
| --- | --- | --- |
| Heap allocation | Test-binary global `new`/`new[]`, including aligned forms | `allocationsOnAudioThread` |
| Heap deallocation | Test-binary global `delete`/`delete[]`, including sized/aligned forms | `deallocationsOnAudioThread` |
| Blocking lock entry | Guard hook immediately before a callback mutex entry | `blockingLockAttemptsOnAudioThread` |
| Wait entry | Guard hook before a callback wait primitive | `waitsOnAudioThread` |
| File open | Guard hook at the callback-reachable import/open boundary | `fileOpenEntriesOnAudioThread` |
| File read | Guard hook at the callback-reachable import/read boundary | `fileReadEntriesOnAudioThread` |
| Path resolution | Guard hook before callback path resolution | `samplePathResolutionsOnAudioThread` |
| Sample decode | Guard hook before source-sample decode/import | `sampleDecodeEntriesOnAudioThread` |
| Stream decode | Guard hook before stream-container loading/decoding | `streamDecodeEntriesOnAudioThread` |
| Large-resource destruction | Guard hook before callback destruction of a loaded cache/resource aggregate | `largeResourceDestructionsOnAudioThread` |
| Final shared-ownership release | Guard hook before callback-owned activation/resource final release | `finalSharedOwnershipReleasesOnAudioThread` |
| Callback deadline | Automatic elapsed-time comparison plus deterministic injection | `lastProcessBlockMicros`, `maxProcessBlockMicros`, `overBudgetCallbackCount` |

`largeResourceReleasesOnAudioThread` remains as the compatibility sum of large-resource destruction and
final shared-ownership release. `getAudioThreadViolationCount()` sums prohibited operations;
`getRealtimeGuardFailureCount()` also includes deadline overruns.

## Callback budget profile

Supported Sprint 4 entry configurations are 44.1 kHz and 48 kHz, block sizes 32 through 1024,
up to 128 MIDI/note events per block, and 64 voices per playback context. Preview and Performance have
separate pools, for a combined target of 128 voices. The deadline is the nominal block duration rounded
to the nearest microsecond.

| Block size | 44.1 kHz deadline (microseconds) | 48 kHz deadline (microseconds) |
| ---: | ---: | ---: |
| 32 | 726 | 667 |
| 64 | 1,451 | 1,333 |
| 128 | 2,902 | 2,667 |
| 256 | 5,805 | 5,333 |
| 512 | 11,610 | 10,667 |
| 1024 | 23,220 | 21,333 |

Hosts may supply other configurations, but they are outside this entry-gate budget declaration and do
not count as a supported EG4 maximum-load configuration.

## Callback changes required by the guard

The clean matrix initially found real violations. EG4 therefore:

- replaced lock-taking `MidiMessageCollector` queues with bounded 256-event SPSC note queues;
- parses non-owning `MidiMessageMetadata` bytes rather than materializing an owning message per event;
- drains queued note events directly at the block boundary without copying them into another buffer;
- removes unused per-voice strings;
- uses a pointer-based realtime route over validated immutable models instead of the diagnostic-string
  `RuntimeVoiceRouteResolution` API; and
- uses `string_view` macro lookup so note starts do not construct temporary strings.

The rich route API remains available to non-realtime engine callers.
