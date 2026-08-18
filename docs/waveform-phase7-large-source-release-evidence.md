# Waveform Workbench Phase 7 Evidence

## Decision

Phase 7 is activated. The SFZ non-destructive region editor is qualified across the waveform worker, authoring workspace, Preview, Publish, host state, package, streaming, voice, offline render, standalone-style shell, VST3 host, and large-corpus paths. No source-audio mutation or SFZ writer was added.

## Production change

Prepared playback now builds a fixed-capacity region prewarm plan for each non-resident source. Preparation publishes deduplicated page intents for the playback start, loop head, crossfade tail, and final loop frame. The work remains on the preparation/page-service side; audio callbacks still publish only bounded lock-free miss intents and the message thread performs no sample-file reads.

Preparation metrics now expose accepted and rejected region-prewarm intents. The real-corpus report records those counters, while a deterministic 100,000,000-frame source proves exact deep-offset targets and shared-source deduplication.

## Qualification matrix

| Gate | Result | Evidence |
|---|---|---|
| Region policy and 64-bit boundaries | PASS | Deep near-maximum frame arithmetic, loop/crossfade intent ordering, duplicate removal, and invalid/degenerate loop behavior. |
| Viewer churn and bounded cache | PASS | 500 MiB logical PCM24 stereo source, 96 blocked-worker replacement/selection/zoom requests, newest-project/stamp publication only, 12 deep tiles with bounded eviction, same-path size/mtime invalidation, and active-worker shutdown. Maximum request submission stayed below one 60 Hz frame. |
| Approximately 500 MiB multi-source projection | PASS | 64-source aggregate fixture validates at 499–500 MiB with deep playback/loop boundaries and valid package descriptors. |
| Deep playback prewarm | PASS | Four accepted deduplicated intents at frames 90,000,000; 92,000,000; 97,997,952; and 97,999,999 with the playback start marked imminent. |
| Source immutability | PASS | Authoring UI and package export/open/render lifecycles compare canonical path, size, modification time, and full fingerprint before and after edits, undo/redo, save, repeated shell lifecycle, package compile/open, and voice render. |
| Debug focused and broad regression groups | PASS | Waveform/authoring 8/8 plus isolated integration; host-state/Preview 16/16; Publish/activation 9/9; package/stream/voice/offline 15/15; direct SFZ region contract PASS. |
| Release focused matrix | PASS | 9/9: package v2, Preview shell parity, region policy, waveform preview, preparation/prewarm, activation recovery, project recall, authoring UI, and package export lifecycle. |
| Real corpus and package | PASS | Accurate Salamander: 1,964,042,398 corpus WAV bytes, 637 projected sources, 1,700 zones, 2,631,113,592-byte package, zero zone/group preparation failures, zero normal-profile underrun frames, and 255,008,768-byte peak process working set. |
| Responsiveness and audible paths | PASS | 22 us authoring refresh, 659 us selection, 131 us Preview dispatch, 1.543 s Preview ready, 41 us Publish dispatch, 197 ms Publish ready, 843 ms package load, 180 seconds continuous playback, 3.478 ms maximum concurrent dispatch, 488 us maximum package audio block, and nonzero authored on-screen, host-MIDI, and package magnitudes. |
| External VST3 host | PASS | 12/12 REAPER cases at 44.1/48 kHz and 128/256/512 samples with editor open/closed, 28 inserted MIDI notes per case, 1,624–1,856 nonzero peak observations, and zero nonfinite observations. |
| Creator and migration documentation | PASS | Creator workflow, keyboard reference, one-way conversion statement, exact/normalized/fallback/unsupported SFZ table, schema migration, and large-source behavior are documented in `waveform-region-authoring-guide.md`. |

## Real-corpus details

The Release large-instrument run used the live Accurate Salamander SFZ corpus, bounded 16 KiB source heads, production page-intent workers, immutable render models, and callback-side activation. Full-draft preparation completed in 5,863,113 us; package export completed in 22,015,869 us at 119,168,487 plaintext bytes/s. The zone profile prepared 393 pages with a 2,211 us maximum page read and a 406 us maximum audio callback against a 5,333 us budget. Cache use was 851,968 bytes against a 402,653,184-byte budget.

The source instrument does not author deep region starts or loops, so its region-prewarm count is correctly 0/0. Deep prewarm is qualified independently with the synthetic 500 MiB/100-million-frame fixtures rather than inferred from the piano corpus.

## Harness corrections made during activation

Two Debug tests placed multiple large processor/playback-context objects in one stack frame and could fail with Windows stack-overflow code `0xC00000FD`. Those test-only objects now use heap lifetime, matching the product's owning shells. After correction, the complete affected groups pass serially.

The checked-in REAPER state was bound to the archived `DecentRhapsodyStudio` path and carried obsolete manifest, authored, prepared, and DSP identities. It has been refreshed from the current Release serializer and propagated to the generated REAPER scenarios. The first stale-state run was silent; the final 12-case matrix is fully audible and is not waived.

## Artifacts

- `artifacts/waveform-region-phase7/accurate-salamander-qualification.md`
- `artifacts/waveform-region-phase7/ui-responsiveness.json`
- `artifacts/waveform-region-phase7/host-matrix.md`
- `artifacts/waveform-region-phase7/current-reference.hoststate.json`
- `docs/waveform-region-authoring-guide.md`

The licensed-data-derived 2.63 GB package was generated in the system temporary directory, used by the large-corpus and responsiveness runs, and deleted after qualification. It is not checked in.

