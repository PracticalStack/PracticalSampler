# analyse.oscilloscope - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/AnalyserNodes.h:353-405`
**Base class:** `analyse_base<Helpers::Oscilloscope>` (template instantiation)
**Classification:** analysis

## Signal Path

Audio passes through unmodified (passthrough). The node writes audio to a SimpleRingBuffer for waveform display. When MIDI note-on events are received, the buffer size is dynamically resized to show exactly one cycle of the note frequency.

Process flow: input audio -> analyse_base<Oscilloscope>::process() -> updateBuffer() -> SimpleRingBuffer::write(). Optional: MIDI note-on -> handleHiseEvent() -> calculate cycle length -> setMaxLength()

## Gap Answers

### signal-path-passthrough: Does oscilloscope pass audio through?

Yes, pure passthrough. The process() method (lines 482-486) calls updateBuffer() without modifying the input signal.

### midi-sync-mechanism: How does optional MIDI sync work?

Enabled via isProcessingHiseEvent() returning true for Oscilloscope specialization (line 445: constexpr templated to true for Helpers::Oscilloscope). In handleHiseEvent() (lines 467-475), when a note-on is received, the frequency is extracted (e.getFrequency()), and the ring buffer's max length is set to display exactly one cycle: numSamplesForCycle = 1.0 / frequency * samplerate, then rb->setMaxLength(). This dynamically resizes the display window to show one period. The cppProperties should include IsProcessingHiseEvent: true for this node.

### channel-handling: How does oscilloscope handle stereo?

Oscilloscope accepts stereo input but displays configurable channels. Helpers::Oscilloscope::NumChannels is constexpr 2 (line 387), indicating stereo capable. The ring buffer is initialized with NumChannels from properties (line 368, default 1) and can be set to 1-2 (validateInt line 396). The node can display one or two channels independently.

### buffer-size: What is the default buffer size?

Default is 8192 samples (set in Oscilloscope constructor, line 381). This is configurable via RingBufferIds::BufferLength property, validated to range 128-65536 (validateInt line 394).

### cpu-profile: Display buffer fill CPU cost?

Audio thread cost is negligible -- simple memcpy to ring buffer. Per-sample updateBuffer() would be more expensive but the node does not implement processFrame(); only process() (block-level). MIDI sync calculation happens in handleHiseEvent() on message arrival (O(1) math operation).

## Parameters

None. All configuration via RingBufferIds properties (BufferLength, NumChannels), not scriptnode parameters.

## Conditional Behaviour

MIDI note-on detection: isProcessingHiseEvent() uses template specialization to return true only for Helpers::Oscilloscope (line 445). When a note-on arrives, handleHiseEvent() calculates and applies cycle-based buffer sizing.

Channel property validation: NumChannels validated to 1-2 range (line 396).

Buffer length property validation: BufferLength validated to 128-65536 (line 394).

## Polyphonic Behaviour

Not polyphonic. MIDI event handling is monophonic -- sets a single max length for the entire buffer, not per-voice.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The oscilloscope uses display_buffer_base<true> inherited from analyse_base. MIDI sync is the unique feature -- on each note-on, the buffer window is resized to show one cycle of the fundamental frequency. This enables users to "lock" the oscilloscope display to the pitch being played.

The cppProperties for this node must have IsProcessingHiseEvent: true to enable MIDI event routing. Existing phase3 doc correctly mentions dynamic buffer resizing in MIDI context.
