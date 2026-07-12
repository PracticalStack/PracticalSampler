Enables fixed-capacity preallocation for slider-pack storage to reduce reallocations during size changes.

This is useful when slider count changes frequently but has a known upper bound.

> [!Warning:Slider count capped by preallocation] While preallocation is active, effective slider count cannot exceed the configured maximum.
