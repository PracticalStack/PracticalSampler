Buffer::decompose(Double sampleRate, JSON configData) -> Array

Thread safety: UNSAFE
Runs sinusoidal-transient-noise decomposition and returns output buffers.
Return layout is [sinusoidal, noise, transient?, noiseGrains] with transient included only when enabled.
Dispatch/mechanics: Parses config keys into SiTraNoConverter::ConfigData, runs SiTraNoConverter processing, then builds the return array with conditional transient output and trailing noiseGrains array.
Pair with: applyMedianFilter (pre-smoothing before decomposition), resample (prepare analysis resolution), detectPitch (post-analysis pitch checks)
Anti-patterns:
  - Do NOT use SlowTransientThreshold/FastTransientThreshold key names -- only SlowTransientTreshold/FastTransientTreshold are parsed.
  - Do NOT assume FastTransientTreshold alone is enough -- current parser bug ignores fast thresholds unless slow threshold array also has size 2.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("decompose", lambda)
