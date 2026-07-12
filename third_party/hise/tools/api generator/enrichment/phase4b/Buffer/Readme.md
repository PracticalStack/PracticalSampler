Buffer (object)
Obtain via: Buffer.create(numSamples)

Fixed-size float buffer for in-memory sample processing, analysis, serialization,
and slicing in script code. Buffer methods cover both in-place transforms and
copy/reference workflows depending on the operation.

Primary workflows:
  1. Offline processing and file workflows: analyse/trim/serialize/export pipelines.
  2. Preview and quick audition: prepare temporary sample data and call Engine.playBuffer.
  3. Wavetable synthesis workflows: generate/transform cycle buffers before table loading.
  4. Visualisation pipelines: DisplayBuffer reads plus ScriptShader uniform transport.

Shape rule:
  - Multi-channel audio data in HISE is represented as Array<Buffer> (one Buffer per channel).

Why Buffer instead of Array:
  - Fixed float storage with lower overhead on large numeric blocks.
  - Audio-focused methods (getMagnitude, getPeakRange, resample, trim, etc.).
  - Direct interoperability with buffer-based APIs and zero-copy views (referTo/getSlice).
  - Prefer Array for mixed-type script data; prefer Buffer for sample-oriented numeric data.

Complexity tiers:
  1. Level checks and gates: Buffer.create, getMagnitude, getPeakRange. Fast activity detection and conditional routing.
  2. State persistence: + toBase64, fromBase64. Round-trip lane/state data through JSON using a sentinel for empty entries.
  3. Export workflow integration: + trim, Buffer.referTo. Build efficient output buffers with minimal copying after window scans.

Practical defaults:
  - Use a named fixed analysis window size (for example 256 samples) for repeatable last-active-sample scans.
  - Use an explicit sentinel like "EMPTY" for missing serialized buffers, and only call fromBase64 when payload is not the sentinel.
  - Reuse one scratch Buffer in restore/import loops instead of allocating a new temporary buffer per entry.

Common mistakes:
  - Treating getSlice as a copy -- it returns a shared view, so writes modify the source region.
  - Using unsupported interpolation names in resample -- only WindowedSinc, Lagrange, CatmullRom, Linear, ZeroOrderHold are accepted.
  - Using SlowTransientThreshold/FastTransientThreshold keys in decompose config -- parser expects SlowTransientTreshold/FastTransientTreshold.
  - Calling fromBase64 on placeholder values -- check sentinel first to avoid unnecessary decode/error paths.
  - Checking only range[1] from getPeakRange for activity -- negative-only content can be valid and gets missed.
  - Allocating a new temporary Buffer inside large restore loops -- reuse one scratch buffer for predictable memory behavior.

Example:
  const var bf = Buffer.create(2048);
  bf.normalise(-3.0);

Methods (15):
  applyMedianFilter  decompose            detectPitch
  fromBase64         getMagnitude         getNextZeroCrossing
  getPeakRange       getRMSLevel          getSlice
  indexOfPeak        normalise            resample
  toBase64           toCharString         trim
