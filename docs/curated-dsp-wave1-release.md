# Curated DSP Wave 1 Release Gate

Status: approved for the Wave 1 catalog (Gain, Saturator, Stereo Delay, and Algorithmic Reverb)

## Activation policy

- New schema-5 / authoring-4 projects may activate resolved curated catalog entries at zone, group,
  and instrument scope after snapshot, graph, and prepared-playback validation succeeds.
- Schema-4 migration preserves slot IDs and order but converts legacy effect metadata into an
  explicitly bypassed, `legacyInert` review state. Migration alone therefore cannot change legacy
  audio. Enabling an effect is a normal schema-5 authoring transaction with catalog parameters.
- Unknown type/version records remain serialized, visible to the creator, and bypassed. They never
  receive a guessed executable implementation or a positional macro binding.
- Snapshot/graph/prepare validation rejects malformed, unsupported, duplicate-owner, or over-budget
  authored graphs before a new activation is staged. A failed replacement retains the last matching
  valid activation where the host-recovery contract permits it.
- Rollback is an execution policy: mark chains bypassed or return effects to unavailable/legacy
  review state. Authored slot IDs, versions, and parameter records remain intact for later recovery.

## Gate evidence

| Requirement | Evidence |
|---|---|
| Configuration matrix and ownership safety | Sprint 4 entry matrices, render/kernel/scheduler/lifecycle/context vectors, and shell parity |
| Timing, no-DSP overhead, retention | `drs_phase1_benchmark_scene` and `curated-dsp-s15-baseline.json` |
| Long activation/diagnostic soak | `drs_sprint4_concurrency_soak_tests` and realtime guard matrices |
| Offline reviewed output | 21 deterministic offline scenarios across 32/64/127/256/512/1024 frame partitions |
| Host and recovery lifecycle | host-state codec, project recall, restore stress, and REAPER evidence |

The release gate is valid only with the focused effect vectors and the complete `drs_all_tests`
configuration passing for the release build under test.
