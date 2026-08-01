# 12-Control Performance Mixer Contract

Status: Sprint 0 contract, July 31, 2026.

This document freezes the host-topology and capacity rules for the 12-control
Performance mixer before the topology implementation begins. It supplements the
[Sprint 6 published macro and automation contract](phase1-sprint6-published-macro-automation.md).
Where the earlier document describes stable authored-ID migration, this document
defines the permanent host surface that those IDs bind to.

## Capacity

- A project may publish at most **12** controls with `exposedInPerformance=true`.
- A project may contain at most **16** authored macros in total.
- Exposed macros receive available slots before hidden helpers.
- These limits are product policy. They are distinct from the 16 physical host
  parameters and must not be inferred from the current project or reference manifest.

## Permanent ordered host slots

| Host order | Host parameter ID | Contract |
| --- | --- | --- |
| 1 | `macro.tone` | Existing compatibility ID; never renamed or reordered. |
| 2 | `macro.motion` | Existing compatibility ID; never renamed or reordered. |
| 3 | `macro.slot.3` | Permanent generic slot. |
| 4 | `macro.slot.4` | Permanent generic slot. |
| 5 | `macro.slot.5` | Permanent generic slot. |
| 6 | `macro.slot.6` | Permanent generic slot. |
| 7 | `macro.slot.7` | Permanent generic slot. |
| 8 | `macro.slot.8` | Permanent generic slot. |
| 9 | `macro.slot.9` | Permanent generic slot. |
| 10 | `macro.slot.10` | Permanent generic slot. |
| 11 | `macro.slot.11` | Permanent generic slot. |
| 12 | `macro.slot.12` | Permanent generic slot. |
| 13 | `macro.slot.13` | Permanent generic slot. |
| 14 | `macro.slot.14` | Permanent generic slot. |
| 15 | `macro.slot.15` | Permanent generic slot. |
| 16 | `macro.slot.16` | Permanent generic slot. |

The processor constructs these 16 parameters once. Project load, edit, publish,
close, and restore may only alter bindings and values; they must not change count,
order, or IDs. Slot numbers are one-based in the product contract and zero-based
only within callback arrays.

## Binding and failure policy

Authored stable IDs are separate from host parameter IDs. A rename or reorder retains
the binding; deleting an authored ID retires it; a new ID receives a free slot
deterministically. A failed replacement keeps the last-known-good published generation
active. Both the plug-in and standalone Performance surfaces must show the most specific
failure code, path, and repair guidance.

`drs_performance_mixer_s0_red_tests` is the contract seam runner. Sprint 1 promotes
the three-, twelve-, and sixteen-binding cases plus lifecycle topology stability to
registered green tests. Sprint 2 promotes the 13-exposed and 17-total preflight
limits, structured-DSP target validation, shell diagnostic parity, and a failed-then-
successful recovery sequence.

## Published presentation model

Every assigned binding carries an immutable presentation record: authored label,
section/source label, parameter label, value unit, control kind, authored order,
and accessibility description. The record is derived during publication from the
captured playback snapshot and the curated DSP catalog. Group-bus targets use the
published group display name, zone-bus targets use the published zone display name,
and all other targets fall back to `Instrument`.

Curated gain parameters use a dB fader; boolean parameters use toggles; all other
parameters use knobs. The Performance surface orders exposed controls by authored
order, never by host slot. These derived labels are deliberately not serialized as
authoring overrides: a group rename appears only after the next successful Publish,
while its fixed host slot and current value migrate unchanged. Missing legacy source
metadata remains publishable using deterministic `Instrument` / `Control` fallback
labels.
