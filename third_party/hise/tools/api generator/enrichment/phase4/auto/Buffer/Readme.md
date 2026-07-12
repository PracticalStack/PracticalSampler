# Buffer

Buffer is a fixed-size float array for in-memory sample data. It is a native script type for audio-style operations where you need predictable numeric storage, fast scanning, and compact state transfer.

Use `Buffer` mainly for these workflows:

1. **Offline processing and file workflows** - analyse, trim, serialise, and export audio data (`loadAsAudioFile`, `trim`, `toBase64`, `writeAudioFile`).
2. **Preview and quick audition** - prepare temporary audio data and play it directly with `Engine.playBuffer`.
3. **Wavetable synthesis workflows** - generate or transform cycle/sample arrays before handing them to wavetable APIs.
4. **Visualisation pipelines (DisplayBuffer + Shader)** - read display data and push numeric arrays to GPU uniforms (`ScriptShader.setUniformData`).

`Buffer` is usually a better fit than `Array` for sample data because it uses fixed float storage, has lower per-element overhead for large numeric blocks, includes audio-focused methods (`getMagnitude`, `getPeakRange`, `resample`, `trim`), integrates directly with buffer-based APIs, and supports zero-copy views via `referTo` / `getSlice`.

Use `Array` for mixed-type script data, and `Buffer` for sample-oriented numeric processing.

## Where Buffer comes from

- `Buffer.create(size)` allocates an independent buffer.
- `Buffer.referTo(source, offset, length)` creates an aliasing view into existing data.
- `Buffer.getSlice(offset, length)` returns another aliasing view into the source buffer.
- `File.loadAsAudioFile()` returns Buffer data (mono returns a Buffer, multi-channel returns an Array of Buffer).
- Practical shape rule: multi-channel audio in HISE is represented as an `Array` of `Buffer` objects (one Buffer per channel).
- `SliderPack.getDataAsBuffer()` / `SliderPackData.getDataAsBuffer()` expose slider-lane data as Buffer.
- `DisplayBuffer.getReadBuffer()` returns a read view over display/ring-buffer content.
- `UnorderedStack.asBuffer(...)` exposes stack storage as a Buffer view.

```js
const var working = Buffer.create(2048);
const var view = Buffer.referTo(working, 256, 512);
```

Writing into `view` changes the same memory region in `working`.

## Working with Buffer

```js
const var b = Buffer.create(8);

// Array-style access
b[0] = 0.25;
Console.print(b.length); // 8

// Range-style iteration
for (s in b)
    s = s + 0.1;

// Vector operations (in-place)
b * 2.0;
b + 0.5;

// Copy / fill operators
const var copy = Buffer.create(8);
b >> copy;
0.0 >> b;

// Optional range arguments on analysis methods
local fullPeak = copy.getMagnitude();
local partialPeak = copy.getMagnitude(2, 4);
```

`normalise()` mutates the source buffer, while `trim()`, `resample()`, and `applyMedianFilter()` return a new Buffer.

## Integration patterns

- Gate activity with `getPeakRange()` before calling `toBase64()` so silent lanes become lightweight sentinels.
- Restore lanes with `fromBase64()` into a scratch buffer, then push values through `SliderPack.setAllValuesWithUndo()`.
- Scan with `getMagnitude()`, then `trim()` both channels to a shared end-point before exporting with `File.writeAudioFile()`.
  Multi-channel export/playback paths use `Array<Buffer>` channel lists.
- Push fixed-size control/state arrays to shaders with `ScriptShader.setUniformData()` for visualisation pipelines.
- Feed prepared mono or multi-channel buffers directly into `Engine.playBuffer()` for audition/playback.

> [!Tip:Most operations mutate in place] Most Buffer operations mutate in place unless they explicitly return a new Buffer. For sparse serialisation, use an explicit sentinel such as `"EMPTY"`, decode only non-sentinel values, and reuse one scratch buffer in loops to avoid repeated allocations.

## Common Mistakes

- **getSlice returns aliasing view**
  **Wrong:** Assuming `getSlice()` returns a copy and editing it directly
  **Right:** Copy the slice into a new `Buffer.create(...)` target before editing
  *`getSlice()` is an aliasing view. Writing to the slice changes the same region in the source buffer.*

- **Use exact resample mode strings**
  **Wrong:** Passing unsupported interpolation text such as `"Spline"` to `resample()`
  **Right:** Use one of the supported mode names (`"WindowedSinc"`, `"Lagrange"`, `"CatmullRom"`, `"Linear"`, `"ZeroOrderHold"`)
  *Unknown mode names throw a runtime error, so validate or centralise mode strings.*

- **Use Treshold spelling in decompose config**
  **Wrong:** Using `SlowTransientThreshold` / `FastTransientThreshold` in `decompose()` config
  **Right:** Use the implemented keys `SlowTransientTreshold` and `FastTransientTreshold`
  *The parser expects the `Treshold` spelling exactly. The usual English spelling is ignored.*

- **Check fromBase64 return value**
  **Wrong:** Calling `fromBase64()` on placeholders and ignoring the return value
  **Right:** Check for a sentinel first, then verify `fromBase64(...) == 1`
  *Sparse payloads often include placeholders. Guarding decode calls keeps restore paths predictable.*

- **Check both getPeakRange values**
  **Wrong:** Checking only `range[1]` from `getPeakRange()` to decide whether a lane is empty
  **Right:** Check both `range[0]` and `range[1]`
  *A lane can contain valid negative-only values and still have no positive peak.*
