# Scriptnode Index Type System

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/snex_basics/snex_IndexTypes.h`
- `hi_dsp_library/snex_basics/snex_IndexLogic.h`
- `hi_dsp_library/snex_basics/snex_Math.h`
- `hi_dsp_library/dsp_nodes/CoreNodes.h`
- `hi_dsp_library/dsp_nodes/CableNodes.h`
- `hi_dsp_library/dsp_nodes/MathNodes.h`
- `hi_dsp_library/dsp_basics/AllpassDelay.h`

---

## 1. Overview

The `snex::Types::index` namespace provides a composable type system for safe
array access. The system has three layers that stack on top of each other:

1. **Logic types** -- define out-of-bounds behavior (wrap, clamp, unsafe, loop)
2. **Index types** -- integer or float wrappers around a logic type
3. **Interpolation types** -- add sample interpolation on top of float indexes

These compose via template nesting. A fully specified index reads inside-out:

```
index::lerp<index::normalised<double, index::clamped<0>>>
         ^              ^         ^            ^       ^
   interpolation    scaling   precision   OOB logic  upper limit (0=dynamic)
```

All index types live in `snex::Types::index` (namespace `snex { namespace Types {
namespace index { ... }}}`).

---

## 2. Logic Types (Out-of-Bounds Behavior)

Each logic type is a template struct parameterized by `UpperLimit` (int).
When `UpperLimit == 0`, the limit is determined at runtime from the container
size ("dynamic bounds"). When `UpperLimit > 0`, the limit is baked in at
compile time for zero-overhead bounds checking.

### wrapped_logic<UpperLimit>

Wraps indexes using modulo arithmetic. Uses `hmath::wrap()` internally.

- `hasBoundCheck() = true`
- `redirectOnFailure() = false`
- Behavior: `value % limit` (with negative index handling, see Section 7)
- Use case: ring buffers, circular access, oscillator phase wrapping

### clamped_logic<UpperLimit>

Clamps to `[0, limit-1]`. Uses `jlimit()`.

- `hasBoundCheck() = true`
- `redirectOnFailure() = false`
- Behavior: `jlimit(0, limit-1, value)`
- Use case: lookup tables, slider packs, any bounded data access

### unsafe_logic<UpperLimit, Offset>

No bounds checking at all. Debug-only `jassert` verifies in-bounds access.

- `hasBoundCheck() = false`
- Offset template parameter shifts the access index by a fixed amount
- Use case: performance-critical inner loops where bounds are guaranteed

### looped_logic<UpperLimit>

Like wrapped, but supports a sub-range loop within the data. Has mutable state
(start, length) set via `setLoopRange(start, end)`.

- `hasBoundCheck() = true`
- Behavior: values below `start` are clamped to `max(0, value)`. Values at or
  above `start` wrap within the loop region `[start, start+length)`
- Use case: audio file playback with loop points

**Note:** `zeroed` is mentioned in the `integer_index` doc comment as a valid
logic type ("when the index is invalid, a zero value will be used") but no
`zeroed_logic` struct exists in the source. It is not implemented.

---

## 3. Integer Index Types

Type aliases defined in `snex_IndexLogic.h` (line 186-190):

| Alias | Expands To | Notes |
|---|---|---|
| `index::wrapped<N, Check>` | `integer_index<wrapped_logic<N>, Check>` | Modulo wrapping |
| `index::clamped<N, Check>` | `integer_index<clamped_logic<N>, Check>` | Saturating clamp |
| `index::unsafe<N, Check>` | `integer_index<unsafe_logic<N, 0>, false>` | No bounds check, CheckOnAssign always false |
| `index::previous<N, Check>` | `integer_index<unsafe_logic<N, -1>, false>` | Offset=-1, accesses element before current |
| `index::looped<N, Check>` | `integer_index<looped_logic<N>, Check>` | Sub-range looping |

### Template parameters

- `N` (UpperLimit): compile-time upper bound. `0` = dynamic (determined at
  access time from container size)
- `Check` (CheckOnAssign): when true AND bounds are not dynamic, the logic
  type's bounds check runs on every assignment/increment. When false, the
  check only runs at access time (`getFrom()`).

### Key behaviors

- `operator=(int)` -- assigns value, optionally checks bounds
- `operator++/--` (pre/post) -- increment/decrement with optional bounds check
- `getFrom(container)` -- returns `container[checked_index]`
- `operator int()` -- explicit cast, only works with compile-time bounds
- `next(containers...)` -- increment and return true if still in bounds
  (only for unsafe indexes)
- `setLoopRange(start, end)` -- only valid for looped indexes

---

## 4. Float Index Types

Type aliases defined in `snex_IndexLogic.h` (line 191-192):

| Alias | Expands To | Notes |
|---|---|---|
| `index::normalised<FT, IntIdx>` | `float_index<FT, IntIdx, true>` | Input 0.0-1.0, scaled to container size |
| `index::unscaled<FT, IntIdx>` | `float_index<FT, IntIdx, false>` | Input is raw sample position |

### Template parameters

- `FT` (FloatType): `float` or `double` precision
- `IntIdx` (IntegerIndexType): the underlying integer index type that provides
  bounds logic

### Normalised vs Unscaled

**Normalised (`IsNormalised = true`):**
- Input range: 0.0 to 1.0
- Internally multiplied by the container size (or compile-time UpperLimit)
- Used by: table lookups (cable_table, core.table, math.table) where the
  input signal is a normalised modulation value

**Unscaled (`IsNormalised = false`):**
- Input is a raw sample position (e.g., 0.0 to 44100.0)
- No scaling applied
- Used by: audio file playback (core.file_player), delay lines, granulators

### Key methods

- `getIndex(limit, delta)` -- returns the integer index after scaling and
  bounds checking, with an optional delta offset (used by interpolation)
- `getAlpha(limit)` -- returns the fractional part for interpolation
- `getFrom(container)` -- returns `container[checked_index]`
- `setLoopRange(start, end)` -- forwarded to underlying logic type

---

## 5. Interpolation Types

Interpolation wraps a float index and reads multiple adjacent samples to
compute an interpolated value. Defined in `snex_IndexTypes.h`.

### index::lerp<IndexType> -- Linear Interpolation

Reads 2 adjacent samples and blends by the fractional part:

```
result = v1 + (v2 - v1) * alpha
```

- Fetches indices at delta 0 and delta +1
- `canReturnReference() = false` (returns a computed value, not a reference)
- Works with both scalar (`span<float, N>`) and compound containers
  (e.g., `span<span<float, C>, N>` for multichannel audio)

### index::hermite<IndexType> -- Cubic (Hermite) Interpolation

Reads 4 adjacent samples (delta -1, 0, +1, +2) for cubic interpolation:

```
a = (3*(x1-x2) - x0 + x3) * 0.5
b = x2 + x2 + x0 - (5*x1 + x3) * 0.5
c = (x2 - x0) * 0.5
result = ((a*alpha + b)*alpha + c)*alpha + x1
```

- Higher quality than linear, especially for pitch-shifted audio playback
- Used by: `StretchNode` (time stretching), audio file playback with pitch

### No interpolation

When no interpolation wrapper is used, the float index returns the element at
the truncated integer position (via `getFrom()`). This is effectively
nearest-neighbor / sample-and-hold.

### Important constraint

Interpolated lookups return computed values, not references. You cannot write
through an interpolated index:

```cpp
auto x = data[lerpIndex];   // OK: returns computed float
auto& x = data[lerpIndex];  // COMPILE ERROR: can't return reference
```

---

## 6. Composition Patterns Used by Nodes

### Table lookup (normalised input, linear interpolation)

Used by `core.table`, `cable_table`, `math.table`:

```cpp
// SAMPLE_LOOKUP_TABLE_SIZE = 512
using TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE>;
using InterpolatorType = index::lerp<index::normalised<double, TableClampType>>;

// Usage: normalised input [0,1] -> interpolated table value
InterpolatorType ip(inputValue);  // inputValue in 0.0..1.0
auto result = tableData[ip];
```

The table is always 512 floats (`SAMPLE_LOOKUP_TABLE_SIZE`). The normalised
index scales 0.0-1.0 to 0-512, clamped indexes prevent out-of-bounds, and
linear interpolation smooths between discrete table entries.

### Slider pack lookup (normalised input, no interpolation)

Used by `cable_pack`:

```cpp
using IndexType = index::normalised<double, index::clamped<0>>;

// Dynamic bounds (0) because slider packs have variable size
IndexType index(inputValue);  // inputValue in 0.0..1.0
auto result = packData[index];
```

No interpolation -- slider pack values are discrete steps. The clamped<0>
uses dynamic bounds because slider packs can be any size.

### Audio file playback with signal input (normalised, clamped, lerp)

Used by `core.file_player` in SignalInput mode:

```cpp
using IndexType = index::normalised<float, index::clamped<0, true>>;
using InterpolatorType = index::lerp<IndexType>;

// Signal drives position: input sample value [0,1] -> audio position
InterpolatorType ip(signalValue);
auto frame = audioData[ip];  // returns multichannel frame
```

### Audio file playback with pitch ratio (unscaled, looped, lerp)

Used by `core.file_player` in PitchRatio mode:

```cpp
using IndexType = index::unscaled<double, index::looped<0>>;
using InterpolatorType = index::lerp<IndexType>;

InterpolatorType ip(uptime * globalRatio);
ip.setLoopRange(loopStart, loopEnd);
auto frame = audioData[ip];
```

The uptime is a raw sample counter (unscaled), with loop points defining the
wrap region within the audio data.

### Granulator (unscaled, clamped, lerp)

Used by `core.granulator`:

```cpp
using IndexType = index::lerp<index::unscaled<double, index::clamped<0>>>;
```

Grain positions are raw sample offsets into the audio file. Clamped to prevent
reading past the end. Linear interpolation for smooth playback at arbitrary
rates.

### Ring buffer / delay (wrapped, compile-time size, lerp)

Used by `AllpassDelay`:

```cpp
using WriterType = index::wrapped<MaxBufferSize, true>;
using ReaderType = index::lerp<index::unscaled<double, WriterType>>;
```

Writer uses integer wrapped index (ring buffer write position). Reader adds
float interpolation for fractional delay times.

### math.pack (normalised, dynamic, lerp)

Used by `math.pack` (alias for `complex_data_lut<0>`):

```cpp
// DataSize=0 -> dynamic bounds
using TableClampType = index::clamped<0>;
using InterpolatorType = index::lerp<index::normalised<float, TableClampType>>;
```

Same as table lookup but with dynamic size. The input signal is normalised
and the pack size determines the actual range at runtime.

---

## 7. Negative Index Handling (hmath::wrap)

The `SNEX_WRAP_ALL_NEGATIVE_INDEXES` preprocessor flag (default: 1) controls
how `hmath::wrap()` handles negative values.

**When enabled (default):**
```cpp
// Negative values wrap correctly around the limit:
wrap(-1, 5) -> 4
wrap(-3, 5) -> 2
// Formula: limit - (abs(value) % limit), then % limit again
```

**When disabled:**
```cpp
// Simple fmod with offset -- only handles values >= -limit:
wrap(value, limit) -> fmod(value + limit, limit)
// wrap(-1, 5) -> fmod(4, 5) -> 4  (works)
// wrap(-6, 5) -> fmod(-1, 5) -> undefined/negative (broken)
```

The default mode handles arbitrarily negative values correctly. This matters
for wrapped indexes used in ring buffers or oscillator phase calculations.

---

## 8. Key Constants

| Constant | Value | Usage |
|---|---|---|
| `SAMPLE_LOOKUP_TABLE_SIZE` | 512 | Fixed size of all lookup tables |
| `SNEX_WRAP_ALL_NEGATIVE_INDEXES` | 1 (default) | Enable safe negative wrapping |

---

## 9. Dynamic vs Compile-Time Bounds

The `UpperLimit` template parameter controls this:

- **`UpperLimit > 0` (compile-time):** Bounds are known at compile time.
  Enables `operator int()` / `operator FloatType()` casts, and the `next()`
  method without container arguments. The compiler can optimize aggressively
  (e.g., power-of-2 modulo becomes bitwise AND).

- **`UpperLimit == 0` (dynamic):** Bounds come from the container's `size()`
  at access time. Required for slider packs (variable size), audio files
  (variable length), and any runtime-sized data.

**`CheckOnAssign` interaction:**
- `CheckOnAssign = true` + dynamic bounds: NOT allowed (static_assert). The
  check cannot run without knowing the limit.
- `CheckOnAssign = true` + compile-time bounds: bounds check on every
  assignment and increment (early validation).
- `CheckOnAssign = false`: bounds check deferred to access time in `getFrom()`.

**Division-by-zero protection:** When `getFrom()` is called on a dynamic
wrapped index with an empty container (`size() == 0`), the code passes
`jmax(1, c.size())` to avoid division by zero in the modulo operation.

---

## 10. Container Compatibility

Index types work with any container that provides:
- `size()` -- returns element count
- `begin()` -- returns iterator/pointer to first element
- `operator[]` -- subscript access (delegates to `getFrom()` internally)

In scriptnode, the primary containers are:
- `span<float, N>` -- fixed-size array (table data, frames)
- `dyn<float>` (aka `block`) -- dynamic view (slider pack data, audio channels)
- `span<block, N>` -- multichannel audio (array of channel views)

When interpolated indexes are used with compound containers like
`span<block, 2>` (stereo audio), the interpolation iterates each sub-element
independently, producing a complete interpolated frame.

---

## 11. Thread Safety Considerations

Index types themselves are stateless value types (except `looped_logic` which
stores start/length). They do not provide any thread safety.

Thread safety for the underlying data is managed separately:
- `DataReadLock` / `DataTryReadLock` -- RAII locks for reading external data
- Nodes must acquire a lock before using indexes to access external data
- On the audio thread, use `DataTryReadLock` (non-blocking) and check success

See core.md Section 7 for full external data locking details.
