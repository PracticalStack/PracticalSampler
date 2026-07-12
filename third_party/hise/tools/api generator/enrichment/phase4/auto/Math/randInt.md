Returns a random integer in the range [low, high). Safe to call from any thread, including the audio thread.

> [!Warning:Upper bound is exclusive] `Math.randInt(0, 128)` returns values 0-127, never 128. When selecting from an array, use `Math.randInt(0, array.length)`.
