Converts a numeric value to a formatted display string using one of the built-in converter modes. This is the inverse of `Engine.getValueForText()`.

| Mode | Example Input | Example Output |
|------|---------------|----------------|
| `"Frequency"` | 440.0 | `"440 Hz"` |
| `"Time"` | 500.0 | `"500ms"` |
| `"TempoSync"` | 5.0 | `"1/4"` |
| `"Pan"` | 0.0 | `"C"` |
| `"NormalizedPercentage"` | 0.75 | `"75%"` |
| `"Decibel"` | -6.0 | `"-6.0 dB"` |
| `"Semitones"` | 2.0 | `"+2 st"` |

> [!Warning:Invalid mode falls back silently] An unrecognised mode string silently falls back to plain integer conversion with no error.