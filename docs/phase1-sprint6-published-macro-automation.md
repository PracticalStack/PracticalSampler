# Mini Sprint 6.7 Published Macro And Automation Binding Contract

Date: July 20, 2026  
Status: Implemented

## Stable topology and immutable publication

The legacy plug-in host parameter layout is constructed once and never changes when an authored
project adds, removes, reorders, or renames macros. Publication maps authored macros to fixed host
slots by stable authored ID, never by authored position or display name. The 12-control mixer
iteration replaces the legacy manifest-owned slot source with the product-owned 16-slot table;
the fixed table remains bounded to 16 host/authored entries.

The fixed host parameter IDs and the 12-visible/16-total capacity split are frozen in
[the 12-control Performance mixer contract](performance-mixer-12-control-contract.md).

Every controller-authorized Performance activation carries an
`ImmutablePublishedMacroBindingTable`. Its revision and macro-schema digest must equal the
authorized playback payload before staging is allowed. The table records detailed host/authored
bindings plus a fixed-size primitive callback view. Unmatched authored macros are explicitly
unassigned; removed bindings are explicitly retired.

## Migration rules

- A compatible stable ID preserves the current Performance value and clamps it into the newly
  published range.
- Reordering or changing the display name cannot change slot identity.
- A newly assigned or re-added stable ID starts from its authored default.
- A removed stable ID retires deterministically and its host slot becomes inactive.
- Duplicate/empty IDs, invalid/non-finite ranges and defaults, duplicate host slots, missing schema
  identity, or more than 16 authored macros reject the binding table.

## Audio-boundary and automation behavior

The message thread prepares the detailed table and callback view. At the same block boundary that
activates the new sampler model, the audio callback installs only the fixed primitive view and
captures an automation sequence baseline. Parameter changes before the boundary therefore belong
to the old binding. The new schema starts from its migrated publication value; parameter changes
after the boundary address the new active binding.

Per-block render controls derive tone velocity and motion pitch only from assigned slots in the
active view. They alter subsequent note-ons without rebuilding or mutating the immutable render
model. Existing voices retain their original generation, route, pitch, gain, loop, and release
state. A retired/unassigned slot cannot influence Performance audio. Authoring schema edits remain
Draft/Preview-only until an explicit Publish succeeds.

## Realtime and status constraints

The callback performs fixed-array copies, lock-free scalar loads/stores, clamps, and arithmetic.
It does not allocate, change host topology, inspect mutable authored containers, or transfer shared
ownership. Processor diagnostics publish the active macro revision and effective tone/motion
controls alongside the active audio revision for boundary conformance tests.
