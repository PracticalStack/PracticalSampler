# CurveEq - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/effects/fx/CurveEq.h` (483 lines) - full class definition including applyEffect() inline
- `hi_core/hi_modules/effects/fx/CurveEq.cpp` (155 lines) - constructor, getAttribute, setInternalAttribute, createEditor

**Base class:** `MasterEffectProcessor`
**Additional bases:** `ProcessorWithStaticExternalData` (1 DisplayBuffer), `ProcessorFilterStatistics::Holder`

## Signal Path

Audio input (stereo) -> series cascade of all enabled filter bands (processed in 64-sample fixed blocks) -> FFT buffer write (if active) -> audio output (stereo, in-place).

The filter bands are processed sequentially in insertion order. Each band operates on the same buffer, so the output of band N is the input to band N+1. This is a classic series EQ topology. The processing is done in fixed 64-sample sub-blocks to align with the filter coefficient smoothing update rate.

## Gap Answers

### signal-path-order

**Question:** Are filter bands processed in series or parallel?

**Answer:** Series (cascaded). In `applyEffect()` (CurveEq.h:218-249), the outer loop divides the buffer into 64-sample fixed blocks. For each sub-block, a `FilterHelpers::RenderData` is created from the buffer, then all filter bands are iterated with `for (auto filter : filterBands) filter->renderIfEnabled(r);`. Each filter modifies the same buffer in-place, so the output of one filter feeds the next. Band order therefore matters - it determines the cascade sequence.

After all bands have processed, the output is written to the FFT display buffer if active.

### fixed-block-processing

**Question:** What is the fixed internal block size and why?

**Answer:** The fixed block size is 64 samples (`FixBlockSize = 64`), defined as a `static constexpr int` in `applyEffect()` (CurveEq.h:220). This sub-blocking serves the filter coefficient smoothing system. Each `MultiChannelFilter` instance calls `updateEvery64Frame()` internally, which recalculates filter coefficients every 64 samples when parameters are being smoothed. The 280ms smoothing time configured in the StereoFilter constructor (`setSmoothingTime(0.28)`) creates gradual transitions for frequency, gain, and Q changes at this 64-sample granularity.

The CPU impact is minimal - the sub-blocking adds a small loop overhead but ensures smooth parameter transitions without per-sample coefficient recalculation.

### band-management

**Question:** How are bands added/removed? What is the scripting API?

**Answer:** Bands are managed through two methods:

- `addFilterBand(freq, gain, insertIndex)` (CurveEq.h:285-309): Acquires the main controller lock, creates a new `StereoFilter`, sets it to Peak type with the given frequency and gain, acquires a write lock on `bandLock`, inserts it into the `filterBands` OwnedArray, calls `updateParameterSlots()`, and sends a broadcaster message.

- `removeFilterBand(filterIndex)` (CurveEq.h:311-324): Same locking pattern, removes from the array, updates parameter slots, sends broadcaster message.

**Scripting API:** From HISEScript, CurveEq is accessed as a `ScriptingEffect` reference. The scripting layer (ScriptingApiObjects.cpp:3375-3411) exposes special constants for CurveEq:
- `Gain` (0), `Freq` (1), `Q` (2), `Enabled` (3), `Type` (4), `BandOffset` (5)

To address band parameters: `effect.setAttribute(bandIndex * effect.BandOffset + effect.Freq, 2000.0)`. The `BandOffset` constant equals `numBandParameters` (5), so band 0's frequency is index 1, band 1's frequency is index 6, etc.

Band addition/removal is done through the `setDraggableFilterData()` / `getDraggableFilterData()` API methods on ScriptingEffect, or via the editor UI.

### filter-types-detail

**Question:** What filter types are available? Biquad or SVF?

**Answer:** Five filter types are available per band, defined in the `FilterType` enum (CurveEq.h:157-165):
- **LowPass** (0): 1-pole low-pass filter
- **HighPass** (1): 1-pole high-pass filter
- **LowShelf** (2): shelving EQ for the low end
- **HighShelf** (3): shelving EQ for the high end
- **Peak** (4): parametric peak EQ (default for new bands)

By default, filters use `StaticBiquadSubType` (standard JUCE IIR biquad coefficients). The compile-time flag `HISE_USE_SVF_FOR_CURVE_EQ` (default 0) can switch to `StateVariableEqSubType` which sounds better when modulated but is not the default for backward compatibility (CurveEq.h:42-49, 167-171).

Note: `StaticBiquadSubType` internally defines a `ResoLow` (index 5) type, but CurveEq's enum and the `modeNames` array in `getMetadata()` only expose indices 0-4. ResoLow is not available to CurveEq users.

### fft-display

**Question:** How does the FFT display work?

**Answer:** The FFT display uses the `DisplayBufferSource` interface via `ProcessorWithStaticExternalData`, which provides one `SimpleRingBuffer` display buffer (constructor: `ProcessorWithStaticExternalData(mc, 0, 0, 0, 1)` - 0 tables, 0 slider packs, 0 audio files, 1 display buffer).

In the constructor (CurveEq.cpp:43-63):
1. The display buffer is retrieved via `getDisplayBuffer(0)`
2. It is registered as an FFT buffer via `registerPropertyObject<scriptnode::analyse::Helpers::FFT>()`
3. Connected to the global UI updater for display refresh
4. Disabled by default (`setActive(false)`)

In `applyEffect()` (CurveEq.h:234-235): after all filter bands process, the **post-filter** output is written to the FFT buffer: `fftBuffer->write(buffer, startSample, numSamples)`. This only happens if `fftBuffer->isActive()`.

The spectrum analyser can be toggled via `enableSpectrumAnalyser(bool)` (CurveEq.h:276-281), which activates/deactivates the ring buffer and sends a broadcaster message.

The `DraggableFilterPanel` FloatingTile renders this FFT data as a spectrum display behind the filter curve visualisation.

### gain-disabled-for-types

**Question:** Is Gain disabled only in metadata or also in DSP for LowPass/HighPass?

**Answer:** In the metadata (`getMetadata()`, CurveEq.h:136-141), the Gain parameter is marked `asDisabled()` when the band's type is LowPass or HighPass. This disables the UI slider.

At the DSP level, the biquad coefficient calculation in `StaticBiquadSubType::updateCoefficients()` simply ignores the gain value for LowPass and HighPass filter types - these filters inherently have no gain parameter in their transfer function. Setting Gain on a LowPass/HighPass band via scripting will store the value but it will not affect the audio output.

### smoothing-time

**Question:** What is the parameter smoothing time?

**Answer:** Each `StereoFilter` is initialised with `setSmoothingTime(0.28)` (CurveEq.h:178) - 280 milliseconds. This applies to frequency, gain, and Q parameters via `LinearSmoothedValue<double>` in the `MultiChannelFilter` base class (MultiChannelFilters.h:180-182).

Smoothing operates at the 64-sample sub-block rate: coefficients are recalculated every 64 samples via `updateEvery64Frame()` during the smoothing period. This is neither per-sample nor per-audio-block - it is per-64-sample-chunk, giving roughly 1.45ms update intervals at 44.1kHz.

### band-limit

**Question:** Is there a maximum number of bands?

**Answer:** There is no hardcoded maximum. The `filterBands` member is an `OwnedArray<StereoFilter>` (CurveEq.h:473) with no size limit. The description "unlimited bands" is accurate in terms of the code. The practical limit is CPU - each band adds one stereo biquad filter to the cascade, and the cost scales linearly with band count.

## Processing Chain Detail

1. **Sub-block division** (per-buffer): The incoming audio buffer is divided into 64-sample chunks. CPU: negligible.

2. **Band lock acquisition** (per-sub-block): A `SimpleReadWriteLock::ScopedReadLock` is acquired to protect the band array from concurrent modification. CPU: negligible.

3. **Filter cascade** (per-sub-block): Each enabled filter band processes the sub-block in series via `renderIfEnabled()`. Each band is a stereo biquad (or SVF) filter with 280ms parameter smoothing. CPU: medium per band (biquad coefficient calculation + per-sample filtering for 2 channels).

4. **FFT buffer write** (per-buffer, optional): If the spectrum analyser is active, the post-filter output is written to the ring buffer. CPU: low.

## Modulation Points

None. CurveEq has no modulation chains. All parameter changes are direct (via scripting setAttribute or the editor UI).

## Interface Usage

### DisplayBufferSource (via ProcessorWithStaticExternalData)

Provides one `SimpleRingBuffer` configured as an FFT analyser. Fed from the post-filter output in `applyEffect()`. The buffer is inactive by default and can be toggled with `enableSpectrumAnalyser()`. The `DraggableFilterPanel` FloatingTile consumes this buffer for spectrum visualisation.

### ProcessorFilterStatistics::Holder

Provides the `CurveEqFilterStats` implementation that bridges the generic filter drag overlay UI with CurveEq's band management. This allows the `FilterDragOverlay` component and `DraggableFilterPanel` FloatingTile to add/remove/drag bands interactively.

## CPU Assessment

- **Baseline:** medium (depends on band count; a typical 4-6 band EQ is medium)
- **Per-band cost:** medium (stereo biquad filter with coefficient smoothing)
- **Scaling factor:** band count (linear scaling - each additional band adds one stereo biquad)
- **FFT display:** low additional cost when active (ring buffer write only; FFT computation is on the UI thread)
- **Overall:** For typical use (3-8 bands), medium. For extreme configurations (20+ bands), high.

## UI Components

- **CurveEqEditor** (backend only): Full editor with `FilterDragOverlay` for interactive band manipulation
- **DraggableFilterPanel**: FloatingTile content type for embedding the EQ display in custom interfaces. Shows the filter frequency response curve, draggable band handles, and optional FFT spectrum overlay.

## Notes

- The prettyName "Parametriq EQ" uses a deliberate stylised spelling.
- New bands default to Peak type at the given frequency and gain. Q defaults to 1.0.
- The `bandLock` (SimpleReadWriteLock) protects the filter array for thread safety during add/remove operations. The audio thread acquires a read lock; add/remove acquire a write lock under the main controller lock.
- The module is suspended on silence (`isSuspendedOnSilence() = true`) and has no tail (`hasTail() = false`).
- Serialisation stores `NumFilters` and all band parameters as flat indexed properties (`Band0`, `Band1`, ..., `BandN`) plus the `FFTEnabled` state.
- The `OLD_EQ_FFT` code path (guarded by `#if OLD_EQ_FFT`, currently 0) is dead code from an earlier FFT implementation.
