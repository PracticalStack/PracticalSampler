Appends the value only if it is not already present in the array. Returns the new length. Uses loose comparison for duplicate checking (`1` matches `1.0`). Supports variadic arguments - each is checked independently.

> [!Warning:Pre-allocate for audio thread use] Same reallocation rules as `push()` apply. Call `reserve(n)` in `onInit` before using this in MIDI callbacks.