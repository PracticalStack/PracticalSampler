Splits the buffer into sinusoidal, noise, optional transient, and noise-grain outputs for offline spectral workflows. The return value is ordered as `[sinusoidal, noise, transient?, noiseGrains]`, where `noiseGrains` is always the final array element.

`configData` is case-sensitive and only reads `SlowFFTOrder`, `FastFFTOrder`, `FreqResolution`, `TimeResolution`, `CalculateTransients`, `SlowTransientTreshold`, and `FastTransientTreshold`.

> [!Warning:Use exact Treshold key spelling] The transient threshold keys must use the implemented `...Treshold` spelling exactly. `...Threshold` is ignored.
