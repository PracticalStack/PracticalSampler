# Buffer -- Class Analysis

## Brief
Fixed-size float buffer object for in-memory audio sample processing and analysis.

## Purpose
Buffer provides a script-level float array container optimized for audio sample data operations. It supports in-place math, analysis helpers, serialization, slicing, and resampling without requiring module handles. In the runtime architecture, Buffer behavior is implemented by `VariantBuffer` (a `DynamicObject`) and exposed through a dedicated var buffer type. Buffer instances can be created directly or obtained as references from other data providers such as DisplayBuffer, SliderPackData, and UnorderedStack.

## Details

### Runtime architecture
- `ScriptBuffer` in `ScriptingApiObjects.h` is the API shell, but runtime method behavior comes from `VariantBuffer::addMethods()`.
- The scripting engine registers a global native object: `Buffer` -> `VariantBuffer::Factory(64)`.
- `juce::var` has a dedicated buffer subtype (`VariantType_Buffer`), enabling `isBuffer()` and `getBuffer()` checks.

### Acquisition paths
| Path | What you get | Semantics |
|------|--------------|-----------|
| `Buffer.create(size)` | New buffer | Owns memory |
| `Buffer.referTo(source, offset, length)` | Referenced section | Aliases source data |
| `DisplayBuffer.getReadBuffer()` | Ring-buffer read view | References external display data |
| `SliderPackData.getDataAsBuffer()` | Slider data buffer | References slider model data |
| `UnorderedStack.asBuffer(...)` | Stack data view | References stack storage |

### Mode selectors and config behavior

#### `resample(ratio, interpolationType, wrapAround)`
Interpolation accepts five fixed mode strings and rejects unknown values with a runtime error. See `Buffer.resample` for the full value table and behavior notes.

#### `decompose(sampleRate, configData)`
Config parsing uses a fixed key set from C++ and includes the implemented `...Treshold` spelling for transient arrays. See `Buffer.decompose` for the key schema, output layout, and current parser edge cases.

### Build sensitivity
- `detectPitch` registration is guarded by `HISE_INCLUDE_PITCH_DETECTION`.
- Median filter backend switches by `USE_IPP_MEDIAN_FILTER`.

### Constants registration status
- No `addConstant()` registrations for Buffer.
- No `ADD_TYPED_API_METHOD_*` registrations in the Buffer class shell.

## obtainedVia
`Buffer.create(numSamples)`

## minimalObjectToken
bf

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| None | -- | -- | No Buffer constants are registered in C++ (`addConstant` not used). | -- |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| None | -- | No dynamic constants are exposed for Buffer. |

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `const var b2 = bf.getSlice(100, 200); b2[0] = 1.0; // expect copy only` | `const var b2 = Buffer.create(200); bf.getSlice(100, 200) >> b2;` | `getSlice` returns a referenced view, so writes affect the original source region. |
| `bf.resample(2.0, "Spline", false);` | `bf.resample(2.0, "Lagrange", false);` | Unsupported interpolation names throw at runtime; only five hardcoded strings are accepted. |
| `bf.decompose(44100, { SlowTransientThreshold: [0.9, 0.6] });` | `bf.decompose(44100, { SlowTransientTreshold: [0.9, 0.6] });` | The parser key in C++ is spelled `Treshold`; the expected English spelling is ignored. |

## codeExample
```javascript
const var bf = Buffer.create(2048);
bf.normalise(-3.0);
```

## Alternatives
- `Array` -- dynamic mixed-type container.
- `MidiList` -- fixed 128-slot integer map for note-indexed data.
- `DisplayBuffer` -- handle to processor-owned ring-buffer display data.
- `AudioFile` -- file-backed audio content and loading workflow.
- `FFT` -- frequency-domain analysis/transform object.
- `File` -- disk I/O and persistence handle.

## Related Preprocessors
`HISE_INCLUDE_PITCH_DETECTION`, `USE_IPP_MEDIAN_FILTER`

## Diagrams

### buffer-acquisition-topology
- **Brief:** Buffer Acquisition and Reference Paths
- **Type:** topology
- **Description:** Shows how Buffer objects originate from the global Buffer factory and from provider objects (DisplayBuffer, SliderPackData, UnorderedStack). Distinguishes owning creation (`create`) from reference-returning paths (`referTo`, `getSlice`, provider-backed views).

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- Buffer.resample -- value-check (logged)
