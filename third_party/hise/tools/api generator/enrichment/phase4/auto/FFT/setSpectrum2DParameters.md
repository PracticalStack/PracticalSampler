Configures the Spectrum2D spectrogram renderer from a JSON object. Only the properties included in the object are updated; omitted properties retain their current values.

| Property | Type | Description |
|----------|------|-------------|
| `FFTSize` | Integer | Log2 of the FFT size (7=128 to 13=8192 samples) |
| `Oversampling` | Integer | Oversampling factor (default: 4) |
| `ColourScheme` | Integer | Colour palette: 0=blackWhite, 1=rainbow, 2=violetToOrange, 3=hiseColours, 4=preColours |
| `GainFactor` | Integer | Gain where 1000 = 0.0 dB (default: 1000) |
| `DynamicRange` | Integer | Minimum dB value for display (default: 110) |
| `Gamma` | Integer | Gamma correction percentage, 0-150 (default: 60) |
| `FrequencyGamma` | Integer | Frequency axis gamma, 100-200 (default: 100) |
| `WindowType` | Integer | Window type constant (0-6, same as FFT window constants) |
| `ResamplingQuality` | String | Image resampling quality: `"Low"`, `"Mid"`, or `"High"` |
| `Standardize` | Integer | Standardise output (boolean, default: false) |
