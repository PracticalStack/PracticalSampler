Enables or disables one or more round-robin groups simultaneously. The groupIndex argument accepts three types:

- An integer (one-based, matching other RR functions)
- An array of integers for predefined static ranges
- A `MidiList` where valid entries (`!= -1`) mark enabled groups (the `enabled` argument is ignored)

This is the method to use when you need multiple groups active at once - for example, round-robin repetitions combined with Group XF (crossfade) layers.

> [!Warning:Layer count must match across RR groups] When this feature is active, the crossfade table index is capped to the number of active groups (`groupIndex %= numActiveGroups`). This means the number of dynamic layers must be consistent across all RR repetitions.
