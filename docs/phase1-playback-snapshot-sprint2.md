# Phase 1 Playback Snapshot Contract

This note captures the first Sprint 2 slice for section 6.1 of `engineering-plan.html`: a product-owned immutable playback snapshot and deterministic builder.

## What this slice adds

- `ImmutablePlaybackSnapshot` as the first product-owned draft-to-playback snapshot shape
- `PlaybackSnapshotBuildRequest` and `PlaybackSnapshotBuildResult`
- lifecycle vocabulary for `Idle`, `Preparing`, `Ready`, `Activating`, `Active`, `Failed`, `Superseded`, and `Canceled`
- deterministic snapshot serialization plus a stable `fnv1a64` content digest
- structured findings with severity, code, path, and message

## Snapshot content in this slice

The Sprint 2 snapshot currently freezes:

- sample identities
- macro defaults and targets
- FX slot references
- routing-bus references
- normalized articulation routes
- normalized group routes
- normalized zones
- revision and schema metadata

This is enough to remove the Phase 1 reference instrument from the contract surface for snapshot construction. The builder translates directly from the current authoring project model and does not decode samples.

## Validation behavior

`drs.phase1.playback_snapshot` now proves that:

- repeated builds of the same draft revision are byte-equivalent and share a digest
- authored edits change the digest predictably
- invalid sample references fail with structured findings
- migrated Phase 1 projects fail predictably with `no-playable-zones` instead of inventing Round Robin, mic-position, or SFZ entities
- canceled and superseded build results carry explicit lifecycle states

## Current boundary

This slice intentionally stops at immutable snapshot construction. It does not yet prepare assets, decode samples, activate snapshots, or replace the shell-owned renderer. Those concerns remain for later Sprint 2 and Sprint 3 steps.
