Returns true if the stack contains the specified value. In float mode, matches by exact float equality. In event mode, uses the configured compare function.

> [!Warning:Uses exact float equality comparison] Float comparison uses exact equality. Floating-point precision issues may cause `contains()` to return false for values that appear equal. Stick to integer-range values (note numbers, MIDI CC indices) to avoid surprises.
