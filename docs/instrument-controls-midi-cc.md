# Instrument Controls and MIDI CC

Imported SFZ controller metadata is represented as native Instrument Controls,
not as published Performance macros. A control has a stable ID, normalized
default, presentation metadata, one optional MIDI binding, and one or more
semantic audio targets. The curated Performance macro surface remains limited
to its existing host-facing slots.

The authoring Controls destination has two peer surfaces: Mixer for master,
bus, and kit-piece gain/pan controls, and Parameters for tuning, envelope,
dynamics, and tone controls. MIDI Assignments opens as a full-height drawer
over the same model, so imported controls do not crowd the Performance panel.

## Value path

UI edits and MIDI CC events enter the same normalized runtime state. The
compiled binding table resolves channel scope without strings or allocation;
active voices consume bounded gain, pan, tune, and envelope contributions.
Gain contributions multiply, pan and tune contributions add and clamp, and
hold/decay/sustain values are sampled at note-on. Gain, pan, and tune refresh
active voices with the existing smoothing path.

## Learn and conflicts

Learn arms one destination and accepts the next non-reserved CC. CC64, CC120,
and CC123 remain transport/safety sources. If a source is already assigned,
the authoring transaction must explicitly replace or cancel; assignment
changes are undoable. The Learn action announces its armed destination and
Escape cancels it; a successful learn reflects the learned CC in the action
name. Manual CC and channel selectors, clear, restore-imported, and reset
actions use the same transaction path.

Host MIDI channels are normalized at the plugin boundary from JUCE's
zero-based status nibble to the one-based channel scope used by bindings. An
exact-channel binding therefore responds only on its declared channel; a
different channel is treated as an absent source.

## SFZ import

The importer recognizes labeled/defaulted CCs, supported `*_onccN` and curve
metadata, and projects compatible gain, pan, tune, and amplitude-envelope
targets. `locc`/`hicc`, keyswitches, release triggers, choke groups, and pedal
conditions remain eligibility systems. The import review exposes Controls,
Bindings, Target Coverage, Hidden/Conditional Controllers, Conflicts, and
Unsupported Controller Targets sections. Reported-only findings are preserved
for review; they are not silently discarded.

For Naked Drums GM, the golden projection currently preserves 1,100 playable
zones/samples, 26 controls, 26 bindings, 25 semantic targets, named CC7/10/20/
41/42 controls, and the complete low-to-high velocity bands. It does not
declare tune or envelope-decay `oncc` targets; those laws remain covered by
the synthetic control fixture and are reported as an explicit corpus absence.

## What the `Data` directory contains

`Naked Drums/Data` is the sample payload, not a second control definition. The
SFZ files under `User` point at those compressed FLAC recordings with their
`sample=` opcodes. Each file is a recorded drum source such as a mic position,
kit piece, articulation, or velocity layer; the SFZ region supplies the key,
velocity, tuning, envelope, routing, and CC conditions that select it. A
`*silence` region is an intentional no-audio placeholder. Keep the relative
directory layout intact when moving the instrument so those sample paths remain
resolvable.

## Troubleshooting

If every hit is full volume, inspect the imported control's binding and target:
the CC source must be assigned to the control and the target must be a gain
contribution. A velocity range is independent from gain modulation; an SFZ
instrument can legitimately use both. Use the import report to distinguish a
missing source label, an unsupported target, and a region eligibility
condition.
