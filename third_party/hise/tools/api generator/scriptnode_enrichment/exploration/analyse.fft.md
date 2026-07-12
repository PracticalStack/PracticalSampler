# analyse.fft - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/AnalyserNodes.h:77-351`
**Base class:** `analyse_base<Helpers::FFT>` (template instantiation)
**Classification:** analysis

## Signal Path

Audio passes through the FFT analyser unmodified (pure passthrough analysis). The node writes incoming audio to a SimpleRingBuffer where FFT is computed asynchronously. The node simply copies samples to the ring buffer; analysis computation happens off the audio thread.

Process flow: input audio (1 channel) -> analyse_base<FFT>::process() (line 482) -> updateBuffer() -> SimpleRingBuffer::write() -> FFT PropertyObject processes buffer asynchronously

## Gap Answers

### signal-path-passthrough: Does the FFT node pass audio through unmodified?

Yes. The process() method (lines 482-486) accepts audio data and calls updateBuffer() which writes to the ring buffer without modifying the input signal. The audio is copied to the buffer, not consumed or altered. Pure passthrough analysis.

### display-buffer-mechanism: How does the FFT node fill the display buffer?

Uses SimpleRingBuffer write pattern via display_buffer_base<true> inheritance. Audio is written via rb->write() (line 57) with multichannel pointer array and sample count. Default FFT buffer is 8192 samples (line 114, configurable 1024-32768 via validateInt line 316). Window overlap is applied via addOverlap() (lines 271-275) which scales the ring buffer size. FFT computation (transformReadBuffer, line 333) runs asynchronously in the PropertyObject update cycle, not on the audio thread.

### fft-properties-from-cpp: What FFT properties are configurable?

Nine properties managed by Helpers::FFT PropertyObject (defined lines 91-101):
- BufferLength (1024-32768)
- WindowType (Blackman-Harris default, line 337)
- Overlap (0.0-0.875, line 344)
- DecibelRange ([-100, 0] default, line 341)
- UsePeakDecay (bool, line 350)
- UseDecibelScale (bool, line 340)
- YGamma (0.1-32.0, line 342)
- Decay (0.0-0.99999, line 343)
- UseLogarithmicFreqAxis (bool, line 339)

All are SimpleRingBuffer PropertyObject properties, not scriptnode parameters. Configured via display buffer UI, not node parameter callbacks.

### channel-handling: How does FFT handle stereo input?

FFT is mono-only. Helpers::FFT::NumChannels is constexpr 1 (line 87). The process() template converts any input ProcessData to ProcessData<1> (line 484), extracting only the first channel. Ring buffer is hardcoded to 1 channel (line 328: toFixSize<1>).

### cpu-profile: FFT CPU cost and timing?

Audio thread cost is negligible -- only copying samples to ring buffer (memcpy via FloatVectorOperations, line 57). FFT computation happens asynchronously off-thread via PropertyObject. No per-block processing overhead or parameter-dependent scaling on audio thread.

## Parameters

None. All configuration via PropertyObject (BufferLength, WindowType, Overlap, DecibelRange, UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis). These are display buffer properties, not node parameters.

## Conditional Behaviour

Window type selection (setProperty "WindowType", lines 128-148): validates against available types, updates currentWindow, triggers window recomputation.

Overlap/BufferLength coupling: changing either via setProperty triggers refreshFFTSize() (lines 149-153, 206-212) which resizes the ring buffer.

Display changes (UseDecibelScale, DecibelRange, UseLogarithmicFreqAxis, YGamma): trigger visual updates via sendContentChangeMessage but do not affect audio processing.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The node is a passthrough wrapper around SimpleRingBuffer::PropertyObject. All analysis computation (transformReadBuffer, line 333) runs off the audio thread. This design cleanly separates real-time audio streaming from non-realtime FFT computation.
