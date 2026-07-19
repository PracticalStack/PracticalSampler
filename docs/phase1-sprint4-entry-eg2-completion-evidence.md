# Sprint 4 Entry Gate EG2 Completion Evidence

Completed July 19, 2026. Successful Preview and Performance preparation now produces retained,
const activation payloads that survive worker-result and completion-queue destruction.

## Focused matrix

`drs_sprint4_entry_activation_payload_tests` verifies:

- const renderer-facing snapshot and prepared handles;
- retained decoded PCM after temporary build results leave scope;
- separate Preview and Performance last-known-good pointers;
- failed, canceled, superseded, stale, and identity-mismatched completion rejection;
- preservation of pointer identity, revision, digest, lifecycle eligibility, and audible assets;
- facade retention after worker completion and queue drain;
- successful device-restart preservation and project-close release;
- bounded message-to-audio slot exchange at the block boundary;
- old-voice retention of a retired prepared payload;
- message-owned cleanup after the final voice lease;
- active, pending, retired, reclaimed, and cache-residency metric separation;
- zero large-resource releases on the audio callback.

## Compatibility verification

The EG2 focused executable and these registered Debug regressions pass:

- `drs.phase1.draft_playback_contract`
- `drs.phase1.draft_playback_facade`
- `drs.phase1.realtime_safety`
- `drs.phase1.diagnostics`
- `drs.phase1.prepared_playback`
- `drs.phase1.prepared_playback_worker`

EG5-T1 registers the focused target as `drs.sprint4_entry.activation_payload` and includes it in
`drs_all_tests`; the fresh Debug matrix verifies discovery and execution.
