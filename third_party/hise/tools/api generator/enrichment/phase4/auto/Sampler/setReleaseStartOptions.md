Configures the release start behaviour from a JSON object. The supported properties are:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ReleaseFadeTime` | Integer | 4096 | Fade time in samples (0-44100) |
| `FadeGamma` | Double | 1.0 | Gamma curve shape (clamped 0.0-2.0) |
| `UseAscendingZeroCrossing` | Integer | false | Snap to ascending zero crossing |
| `GainMatchingMode` | String | `"None"` | `"None"`, `"Volume"`, or `"Offset"` |
| `PeakSmoothing` | Double | 0.96 | Peak smoothing factor |

Use `Sampler.getReleaseStartOptions()` to read the current configuration.
