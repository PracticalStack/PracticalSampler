# analyse.goniometer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/AnalyserNodes.h:406-431`
**Base class:** `analyse_base<Helpers::GonioMeter>` (template instantiation)
**Classification:** analysis

## Signal Path

Audio passes through unmodified (passthrough). The node writes stereo audio pairs to a SimpleRingBuffer where stereo correlation (Lissajous X-Y plot) is visualized. The display shows spatial relationship between left and right channels.

Process flow: stereo input -> analyse_base<GonioMeter>::process() -> updateBuffer() -> SimpleRingBuffer::write(2 channels)

## Gap Answers

### signal-path-passthrough: Does goniometer pass audio?

Yes, pure passthrough. The process() method calls updateBuffer() without modifying input.

### stereo-correlation-method: How is stereo correlation computed?

The node writes stereo pairs (left and right channels) to the ring buffer. The GonioMeter PropertyObject::createPath() and simple_gon_display::paintSpacialDots() (lines 586) render an X-Y (Lissajous) plot where left channel maps to X axis and right channel maps to Y axis. This visualizes phase and amplitude correlation between channels. Mono (summed) signal appears as a diagonal line; anti-phase signals appear as horizontal/vertical lines; stereo signals fill the plane.

### channel-requirement: Does goniometer require stereo?

Yes, explicitly stereo-only. Helpers::GonioMeter::NumChannels is constexpr 2 (line 418). The ring buffer is hardcoded to 2 channels (line 427: toFixSize<2>). Mono input will be duplicated or rejected.

### cpu-profile: Display buffer write cost?

Audio thread cost is negligible -- memcpy of two channels to ring buffer per block. No per-sample processing; analyse_base does not implement processFrame(). MIDI sync not supported (goniometer only analyzes spatial data, not time-domain sync).

## Parameters

None. All configuration via RingBufferIds properties (BufferLength, NumChannels fixed to 2), not scriptnode parameters.

## Conditional Behaviour

Channel count validation: NumChannels hardcoded to 2 via toFixSize<2> (line 427).

Buffer length validation: validated 512-32768 samples (line 425).

## Polyphonic Behaviour

Not polyphonic.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The goniometer is the simplest analyse node -- it does not recompute FFT or calculate cycle-based sync like oscilloscope. It is a pure stereo visualization tool. The X-Y plot (Lissajous) representation is created in GoniometerBase::paintSpacialDots() via the ring buffer component infrastructure.

Existing phase3 doc is a stub; this exploration provides the complete signal flow.
