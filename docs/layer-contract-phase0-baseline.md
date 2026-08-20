# Layer contract Phase 0 baseline

Status: complete  
Scope: `PracticalSampler` only. The sibling `_analysis/Rhapsody/` tree is out of scope.

## Current architecture inventory

Practical Sampler currently models a two-level authoring and playback hierarchy:

```text
zone -> group -> master
```

- `RuntimeProjectZoneDefinition` stores the child-side `groupId` relationship and zone-local properties such as gain, pan, key/velocity ranges, and playback behavior.
- `RuntimeProjectGroupDefinition` is an independent persisted object. It stores identity, display order/visibility, gain, pan, routing, and an audition anchor; group edits are not written into member-zone gain or pan values.
- `RuntimeProjectAuthoringState` persists selected group state and separate `zones` and `groups` collections. There is no persisted layer collection or group-to-layer relationship yet.
- `PlaybackSnapshotGroupRoute` is the immutable runtime group route. Snapshot construction derives its `zoneIds` membership from each zone's `groupId`, while preserving group-level gain/pan as route values.
- The render model combines zone gain/pan with the corresponding group route and master values at render time. It does not flatten group edits into zones.
- `DspGraphPlan` already has a zone-to-group-to-master routing shape, so the layer iteration must extend the existing route hierarchy without changing the established group contract.

## Import and normalization seams

- SFZ projection creates or reuses explicit groups while projecting zones and applies group-level contributions during import normalization. The layer iteration must add default-layer materialization at this boundary without losing the existing group behavior.
- Authoring-session normalization/migration repairs older projects that do not contain explicit group definitions. The same compatibility seam is the correct place to guarantee a default layer for projects that contain groups but no layers.
- WAV/sample import and any other direct zone-creation path must use the same hierarchy materializer; no importer should be allowed to create an orphan zone.

## UI seams

The current authoring surface exposes group selection and group editing, but no layer selection, layer membership affordance, or layer crossfade controls. The first UI iteration should reuse the group interaction model and make the parent relationship visible before adding advanced layer editing.

## Baseline contract fixtures

- `tests/fixtures/layer-contract/layer-hierarchy.fixture.json` defines one default layer, two groups assigned to it, three zones, and expected membership counts.
- `tests/fixtures/layer-contract/layer-crossfade.fixture.json` defines no-crossfade, velocity-crossfade, controller-crossfade, and invalid missing-partner cases.
- `tests/src/LayerContractTests.cpp` characterizes the current independent group behavior and validates the fixture contract. It deliberately asserts that group gain/pan remain on the group route and that zone-local values remain unchanged.

## Verification

The Phase 0 executable is registered as `drs.layer_contract.phase0` and is built with the repository's normal CMake test targets.

```powershell
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cmake --build build/vs2022-sprint5-clean --target drs_layer_contract_phase0_tests --config Debug -j 4'
ctest --test-dir build/vs2022-sprint5-clean -C Debug -R '^drs\.layer_contract\.phase0$' --output-on-failure
```

The focused Phase 0 test passes after the fixture reader was corrected to construct a string explicitly from the input stream and reject empty fixtures.

## Phase 0 exit decision

The existing group object is a suitable independent parent route. The layer contract can be added as a parallel first-class object with child-side `group.layerId` membership, preserving the established zone-to-group relationship and render-time composition model.

## Phase 1 persisted contract

The native layer schema is project schema `10` with authoring schema `9`.

- `authoring.layers[]` is the first-class layer collection.
- `authoring.selectedLayerId` tracks authoring selection.
- `authoring.groups[].layerId` is required in schema 10 and enforces one group parent per group.
- Each layer carries the group-like display, visibility, gain, pan, routing, and audition-anchor fields.
- `layer.crossfade` is typed as `none`, `velocity`, or `controller`, with a normalized low/high window and an initial `linear` curve. Controller crossfades require a MIDI controller number from 0 through 127.
- Existing schema-9 projects migrate to one `default-layer`, preserve all group-local values, attach every existing group to that layer, and select it.

The application upgrade path now advances schema 8/7 through loop-crossfade schema 9/8 and then into the layer schema 10/9 before project use.

## Phase 2 materialization rule

`materializeProjectLayerHierarchy` is the shared document-boundary repair used by SFZ projection and imported-content append. For layer-schema projects it:

- assigns an ungrouped imported zone to `default-group`;
- creates or reuses `default-layer` and assigns the default group to it;
- assigns imported groups with empty `layerId` to `default-layer`;
- preserves existing group/layer identity, order, gain, pan, routing, and anchors; and
- records authoring notes when default containers are synthesized.

The materializer is also exercised through the generic append path used by WAV import, so the two import workflows share the same hierarchy invariant.
