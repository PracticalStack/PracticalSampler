Executes the callback once for each element. Returns `undefined`. The callback receives up to three arguments: `element`, `index`, and `array`.

> [!Warning:Allocates on the audio thread] `forEach` allocates scope objects internally. Use `for (x in array)` for allocation-free iteration in MIDI callbacks.