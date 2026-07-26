# Phase 3 Round Robin Sprint 1 - Direct-Only Expected-Red Audit

Sprint 1 intentionally freezes the current Round Robin seams before the implementation slices close
them. This note names the expected-red audit that should keep failing until later Phase 3.1.3 work
lands.

## Direct-only audit target

- `drs_phase3_round_robin_contract_red_tests`

This executable is not registered as a green CI test. It is a source-level audit for known open
gaps that Sprint 1 is documenting rather than solving.

## Gaps intentionally left open by Sprint 1

- the native runtime model still stores RR data only as scalar length / position fields
- there is still no explicit pool identity or playback mode in authored or instrument state
- runtime route selection still depends on voice-id modulo behavior instead of pool-scoped counters
- filename import suggestions still expose only a flat `roundRobinIndex`
- the current authoring UI still has no explicit Round Robin editor surface

## Exit condition for this red audit

This audit should stop failing only after later Phase 3.1.3 slices land all of the following:

- explicit RR pool identity in the native model
- runtime pool-scoped counters
- richer import-side RR modeling
- creator-facing RR editing and diagnostics
