# Phase 1 Zone Groups Sprint 1 - Contract Freeze And Migration Baseline

Frozen July 29, 2026.

This note is the Sprint 1 execution artifact for `phase1-6-2-zone-groups-development-plan.html`.
It freezes the first implementation contract for Zone Groups before authored schema, snapshot,
routing, session, and UI changes start landing across the repository.

## Review findings resolved in Sprint 1

Sprint 1 closes the largest remaining ambiguities from the 6.2 plan:

- Groups become explicit authored project objects in a later sprint, but zone membership remains
  zone-owned through the existing required `groupId` field.
- The future authored group object is frozen as owning shared creator-facing state only: identity,
  display naming, UI order, workspace visibility, group mix, routing attachment, audition anchor,
  and group-owned Round Robin policy.
- The canonical routing-source vocabulary is frozen as:
  - `master`
  - `zones/<zoneId>`
  - `groups/<groupId>`
- Group visibility is frozen as workspace-only state. Hiding a group may reduce visual clutter, but
  it must never change snapshot eligibility, digest inputs, prepared playback, published output, or
  live audio routing.
- Group-owned Round Robin is frozen as an ownership and workflow change, not as permission to merge
  incompatible zones into one flat pool.

These decisions give later sprints one stable authored-group contract and one stable migration
story.

## Frozen authored group shape

Sprint 1 freezes the future authored group object with these fields:

- required:
  - `id`
  - `displayName`
  - `displayOrder`
  - `workspaceVisible`
  - `gainDb`
  - `pan`
- optional:
  - `routingBusId`
  - `auditionAnchorZoneId`
  - `roundRobin`

Sprint 1 also freezes these ownership boundaries:

- group membership remains authoritative on zones through required `authoring.zones[].groupId`
- zone-local state remains authoritative for sample choice, key range, velocity range, loop,
  crossfade, gain, pan, and trigger mode
- group state layers shared creator-facing controls above that zone-local state instead of replacing
  it

## Frozen migration rule from legacy zone-only group tags

Legacy projects currently carry only per-zone `groupId` values. Sprint 1 freezes the migration
baseline that later schema work must implement:

- synthesize one authored group object for each distinct legacy `groupId`
- preserve every zone's existing `groupId` value exactly
- order synthesized groups by first zone appearance in `authoring.zones`
- set authored `group.id = zone.groupId`
- default `displayName` to the same string as `id`
- set `displayOrder` to the first-seen group order
- set `workspaceVisible = true`
- set `gainDb = 0.0`
- set `pan = 0.0`
- leave `routingBusId` empty
- set `auditionAnchorZoneId` to the first zone encountered for that group
- leave `roundRobin` absent until the creator explicitly configures group-owned RR later

This migration must preserve route meaning exactly. It is a structural lift from inferred groups to
authored groups, not a sound-changing rewrite.

## Frozen Round Robin ownership boundary

Round Robin ownership moves to the group surface later, but Sprint 1 freezes the compatibility rule
that determines which zones can participate together.

A zone is compatible with another zone for shared RR operations only when all of the following
match exactly:

- `groupId`
- `articulationId`
- `rootKey`
- `keyLow`
- `keyHigh`
- `velocityLow`
- `velocityHigh`
- full `velocityCrossfade` descriptor
- `triggerMode`

This is the current exact-match helper already used by the authoring path in:

- `engine_adapter/src/AuthoringSession.cpp`
- `app/src/shared/AuthoringPanel.cpp`

Sprint 1 freezes that predicate as the baseline rule. Group-owned RR later rehomes the workflow,
but it does not imply that every zone inside one group belongs to one Round Robin pool.

## Current baseline seams recorded by Sprint 1

Sprint 1 intentionally records the current seams rather than closing them immediately:

- `RuntimeProjectAuthoringState` still has no explicit authored `groups` collection
- there is still no `selectedGroupId` in authored project state
- project save/load still persists only zone-owned `groupId` membership
- playback snapshots still synthesize `groupRoutes` directly from zone membership
- snapshot group routes currently carry only `groupId`, `articulationIds`, and `zoneIds`
- routing input selection still exposes only `master` and zone ids
- no group visibility persistence, group manager, group inspector, or group audition surface exists
- RR authoring remains zone-entry UI today even though later ownership moves to groups

Those open seams are documented in:

- `docs/phase1-zone-groups-sprint1-red-tests.md`

and encoded as the direct-only expected-red audit:

- `drs_phase1_zone_groups_contract_red_tests`

## Test coverage frozen by this slice

Green contract coverage:

- `drs.phase1.zone_groups_contract`

Direct-only expected-red coverage:

- `drs_phase1_zone_groups_contract_red_tests`

## Non-goals of Sprint 1

Sprint 1 does not yet:

- add explicit authored groups to the project model
- bump the project schema for group persistence
- expand snapshot or prepared-playback group metadata beyond current membership routes
- allow `groups/<id>` as authored routing inputs
- add group mix, DSP, visibility, audition, or selected-group UI
- move creator-facing RR controls from zones to groups

Those remain intentionally gated by later sprints in the 6.2 roadmap.
