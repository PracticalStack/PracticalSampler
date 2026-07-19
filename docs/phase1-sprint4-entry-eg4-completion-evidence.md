# Sprint 4 Entry Gate EG4 Completion Evidence

Completed July 19, 2026. The real-time guard contract and callback budget table are documented in
`phase1-sprint4-entry-realtime-guard-contract.md`.

## Focused negative matrix

`drs_sprint4_entry_realtime_guard_tests` injects each operation into exactly one callback and requires
its dedicated counter to fail without contaminating another category:

1. allocation;
2. deallocation;
3. blocking lock entry;
4. wait entry;
5. file open;
6. file read;
7. path resolution;
8. sample decode;
9. stream decode;
10. large-resource destruction;
11. final shared-ownership release; and
12. over-budget callback.

The allocation hooks cover scalar, array, sized, aligned, and aligned-array forms in the focused test
binary. The production allocator and resource-release behavior are not replaced.

## Clean maximum-load case

The same executable prepares a 48 kHz / 1024-sample processor, queues the declared 24-note target for
each of the Preview and Performance contexts, supplies a combined 128-event block, and renders audio.
It requires:

- zero allocations and deallocations;
- zero lock and wait entries;
- zero file, path, sample-decode, and stream-decode entries;
- zero large-resource destruction and final shared releases;
- zero voice-capacity growth;
- zero callback deadline overruns;
- a 21,333 microsecond reported deadline;
- the declared combined 48-voice capacity; and
- non-silent output.

## Compatibility verification

The focused EG4 executable and these affected Debug regressions pass:

- `drs.phase1.realtime_safety`;
- `drs_sprint4_entry_activation_payload_tests`; and
- `drs_sprint4_entry_diagnostics_concurrency_tests` with 1,200 callbacks and concurrent UI polling.

EG5-T1 registers the focused target as `drs.sprint4_entry.realtime_guard` and includes it in
`drs_all_tests`; the fresh Debug matrix verifies aggregate discovery and execution.
