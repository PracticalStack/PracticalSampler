Loads audio data into the wavetable synth for resynthesis. The first parameter accepts three input types:

- A **ScriptFile** object (loads from an audio file reference)
- An **Array of Buffers** (multi-channel loading)
- A single **Buffer** (mono loading)

The `loopRange` parameter defines the length of a single cycle as `[startSample, endSample]`. The synth automatically splits the buffer into multiple cycles based on this range, allowing morphing between them.

Call `resynthesise()` after loading data to trigger wavetable generation. The wavetable synth's mip-mapping process handles band-limiting automatically, so generated waveforms do not need manual anti-aliasing.

You can also use an FFT object to build waveforms in the frequency domain via inverse FFT, which is faster than computing samples directly in HiseScript and allows control over individual harmonics.

> [!Warning:ScriptFile ignores sampleRate and loopRange] When loading from a ScriptFile, the `sampleRate` and `loopRange` parameters are silently ignored. The file provides its own metadata.
