Parses a formatted text string back to a numeric value using one of the built-in converter modes. This is the inverse of `Engine.getTextForValue()` and is useful for processing text input from `Content.showModalTextInput()`.

| Mode | Example Input | Example Output |
|------|---------------|----------------|
| `"Frequency"` | `"1.5 kHz"` | 1500.0 |
| `"Time"` | `"1.5s"` | 1500.0 |
| `"TempoSync"` | `"1/4"` | 5.0 |
| `"Pan"` | `"50L"` | -50.0 |
| `"NormalizedPercentage"` | `"75%"` | 0.75 |
| `"Decibel"` | `"-INF"` | -100.0 |
| `"Semitones"` | `"+2 st"` | 2.0 |