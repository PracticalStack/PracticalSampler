Appends one or more values to the end of the array and returns the new length. Supports variadic arguments - each argument is appended individually.

> [!Warning:Pre-allocate for audio thread use] Triggers a reallocation warning on the audio thread when the array exceeds its current capacity. Call `reserve(n)` in `onInit` before using `push` in MIDI callbacks.