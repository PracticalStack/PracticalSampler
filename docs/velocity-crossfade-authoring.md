# Velocity Crossfade Authoring

Velocity crossfades blend adjacent velocity layers with a shared linear overlap. A relationship always owns both sides: the lower layer fades out and the upper layer fades in over the same absolute MIDI velocity range.

## Two layers

Select the two compatible layers in the Zone Map. In **Sample > Velocity Crossfades**, create the relationship, then set its overlap Low and High values. The zones must share articulation, root key, key range, trigger mode, and Round Robin identity.

Use **Apply Overlap** for an exact numeric change, or drag the diamond handles in the Zone Map. **Remove Crossfade** clears only the paired fade descriptors; it does not restore a previous hard split or otherwise change velocity ranges.

## Layer stacks

Select every compatible velocity layer, then choose **Create Stack Crossfades**. The planner orders layers by velocity range and creates exactly one relationship between each adjacent pair. It preserves the stack's outer endpoints, clamps overlap widths when necessary, and commits the entire stack as one undo step.

The inspector preview lists the ordered layers and overlap ranges before creation. If it reports an ambiguous layer order or insufficient room, correct the velocity layers first instead of relying on an ID-based tie-break.

## Round Robin rules

Round Robin layers must be selected as complete bundles. Each velocity layer needs exactly one copy of every pool slot, and every slot must have the same velocity and crossfade shape. Incomplete pools, mixed slot counts, duplicate slots, and partial selections are rejected.

## Audition and troubleshooting

For an existing relationship, **Audition 5 Steps** plays the current draft immediately below the overlap, at the low edge, at the midpoint, at the high edge, and immediately above it. The displayed `L` and `U` values are the runtime lower/upper gain contributions, not an approximation made by the UI.

If audition is unavailable, prepare the draft playback first and confirm that the selected source is playable. A relationship marked needs-review has no unique valid partner; repair its mapping or remove its unsupported metadata before editing.

Supported imported SFZ linear crossfades remain editable. Unsupported or incomplete imported shapes stay review-required and are never silently converted into a different linear relationship.
