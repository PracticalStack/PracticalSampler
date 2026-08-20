# Layer contract policy

This is the active Practical Sampler guidance for the layer hierarchy. New code and new documentation should use this contract. Older HTML documents are historical and are not retrofitted.

## Hierarchy

The authoring hierarchy is:

```text
layer -> group -> zone
```

A layer is an independent authored object. Group and zone values are not rewritten when layer values are edited. Runtime composition applies zone, group, layer, and master values as separate stages.

Every schema-10 project uses `authoring.layers[]`, and every authored group has a non-empty `layerId` that resolves to exactly one layer. `selectedLayerId` follows the active authoring context.

## Default materialization

All supported import and append paths use the shared layer materializer.

- An ungrouped imported zone receives `default-group`.
- The same transaction creates or reuses `default-layer` and assigns the group to it.
- An imported or migrated group without a layer receives `default-layer`.
- Existing layer/group IDs, order, gain, pan, routing, and anchors are preserved.
- Synthesis is recorded in authoring notes.

## Runtime behavior

Neutral layer gain and pan are transparent. Layer gain and pan are composed after the group stage without flattening authored fields. Layer-owned routing uses `layers/<id>` and the DSP graph order is zone → group → layer → master.

Layer crossfade metadata is typed and bounded:

- `source`: `none`, `velocity`, or `controller`;
- `low`/`high`: ordered values from 0 through 127;
- `direction`: `fade_in` or `fade_out`;
- `curve`: currently `linear`.

Crossfade weighting is bounded and allocation-free at playback. Controller values affect subsequent note-on evaluation; scoped preparation retains sibling groups in the active layer so preview behavior matches the full project.

## Authoring expectations

The Map presents layers above groups with group/zone counts. Layer selection, ordering, assignment, visibility, gain, pan, routing, audition anchor, and crossfade controls are document transactions. Group creation defaults to the active layer, and multi-group assignment is available through the session contract.

## Verification

The focused contract set is:

- `drs.layer_contract.phase0`
- `drs.layer_contract.schema_persistence`
- `drs.layer_contract.import_materialization`
- `drs.layer_contract.runtime`
- `drs.layer_contract.authoring`

The app/plugin Debug build must also complete before release evidence is recorded.
