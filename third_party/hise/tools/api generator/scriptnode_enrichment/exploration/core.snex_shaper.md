# core.snex_shaper - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:593`
**Base class:** Template wrapper (no explicit base)
**Classification:** audio_processor

## Signal Path

`snex_shaper<ShaperType>` is a thin template wrapper that delegates all processing to the user-provided `ShaperType` class. The wrapper itself does no audio processing -- it forwards `process()`, `processFrame()`, `prepare()`, and `reset()` directly to `shaper.process(data)`, etc.

The user's SNEX code defines the actual waveshaping algorithm. The default template provides a `getSample(float input)` convenience function that `process()` and `processFrame()` forward to.

## Gap Answers

### shaper-callback-set: Required callbacks?

The wrapper forwards `process()`, `processFrame()`, `prepare()`, and `reset()` unconditionally. These four methods must exist in the ShaperType. `getSample()` is a convention in the default template -- it is NOT a required callback. The user can implement `process()` and `processFrame()` directly without `getSample()`.

### transfer-function-display: How does the display work?

The display is handled by the `SnexShaper` runtime class (in `SnexShaper.cpp`), which passes test samples through the shaper and plots the result. The user does not need to implement a separate display callback -- the existing `process`/`processFrame` callbacks are used.

### template-argument-polyphonic: How does NumVoices work?

The `TemplateArgumentIsPolyphonic` property means when the node is compiled to C++, the ShaperType template receives the voice count as its template parameter (e.g., `MyShaper<1>` or `MyShaper<256>`). This allows the user's SNEX code to use `PolyData<T, NumVoices>` for per-voice state. However, the outer `snex_shaper` wrapper itself is NOT polyphonic (`isPolyphonic()` returns false at line 624).

### external-data-support: Does it support tables/audio files?

Yes, conditionally. `setExternalData()` (line 641) forwards to `shaper.setExternalData()` only if the ShaperType implements it (checked via `prototypes::check::setExternalData`). If the user's SNEX class defines `setExternalData()`, it will receive table/audiofile data.

### no-midi-events: Confirmed no MIDI?

Confirmed. `SN_EMPTY_HANDLE_EVENT` at line 599. The node does not receive MIDI events.

## Parameters

No built-in parameters. User-defined parameters are forwarded via `setParameterStatic<P>()` (line 647) which calls `shaper.setParameter<P>(v)`.

## CPU Assessment

baseline: low (depends entirely on user SNEX code)
polyphonic: false
scalingFactors: []
