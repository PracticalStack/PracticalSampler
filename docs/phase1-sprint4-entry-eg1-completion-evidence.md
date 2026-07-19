# Sprint 4 Entry Gate EG1 Completion Evidence

Completed July 18, 2026. EG1 now prepares general authored WAV and FLAC inputs without requiring
their path or source id to exist in the Phase 1 reference stream.

## Implemented boundary

- Public source fingerprint probing supports cache lookup before full decode.
- Worker request resolution creates an authored-source resolution for every immutable snapshot
  sample, whether or not a stream container is loaded.
- Compiled topology is retained only when its checksum matches current source bytes; otherwise a
  decoded-memory binding is prepared.
- Cache identity uses canonical source identity, actual source fingerprint, effective decode
  policy, and compiler salt.
- Existing frozen Preview and Publish revisions preserve success metrics and actionable worker
  findings without new shell-side decode paths.

## Focused regression matrix

`drs_sprint4_entry_authored_input_tests` passes:

- new WAV: Preview and Publish, cold then warm;
- new FLAC: Preview and Publish, cold then warm;
- preparation with no compiled stream container;
- same-path replacement: only the changed source cold-misses and receives a new fingerprint/key;
- relink with identical bytes: canonical identity changes and only that source cold-misses;
- source removed after snapshot: `prepared-sample-source-missing` reaches Preview;
- unsupported replacement after snapshot: `prepared-sample-format-unsupported` reaches Publish;
- queued cancellation: canceled lifecycle/count, empty queue, and no stale completion.

## Compatibility verification

The following existing Debug regressions pass after the contract change:

- `drs.phase1.draft_playback_contract`
- `drs.phase1.draft_playback_facade`
- `drs.phase1.prepared_playback`
- `drs.phase1.prepared_playback_worker`

EG5-T1 registers the focused executable as `drs.sprint4_entry.authored_input` and includes its
build target in `drs_all_tests`. Fresh-tree discovery and the full Debug matrix verify the
aggregate integration.
