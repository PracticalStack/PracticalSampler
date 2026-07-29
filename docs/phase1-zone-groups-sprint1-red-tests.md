# Phase 1 Zone Groups Sprint 1 - Direct-Only Expected-Red Audit

Sprint 1 intentionally freezes today's group seams before later 6.2 implementation slices replace
them. This note names the expected-red audit that should keep failing until explicit authored-group
work lands.

## Direct-only audit target

- `drs_phase1_zone_groups_contract_red_tests`

This executable is not registered as a green CI test. It is a source-level audit for known open
gaps that Sprint 1 is documenting rather than solving.

## Gaps intentionally left open by Sprint 1

- authored project state still has no explicit `groups` collection or `selectedGroupId`
- project load/save still persists only zone-owned membership through `groupId`
- snapshot group routes are still synthesized from zones and carry membership only
- routing input selection still has no `groups/<groupId>` option
- creator-facing Round Robin controls still live on zone surfaces instead of group surfaces
- there is still no persisted workspace visibility, group audition anchor, or group mix surface

## Exit condition for this red audit

This audit should stop failing only after later 6.2 slices land all of the following:

- explicit authored groups in project state
- deterministic migration from legacy zone-only `groupId` projects
- richer snapshot and prepared-playback group metadata
- canonical `groups/<groupId>` routing support
- creator-facing group visibility, audition, mix, DSP, and RR workflows
