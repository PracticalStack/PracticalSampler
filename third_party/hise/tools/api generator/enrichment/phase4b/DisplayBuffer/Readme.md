DisplayBuffer (object)
Obtain via: Engine.createAndRegisterRingBuffer(index) or Synth.getDisplayBufferSource(moduleId).getDisplayBuffer(index)

Scriptable reference to a processor's ring buffer for real-time audio visualization
and state serialization. Reads audio data written by the audio thread and converts it
to paths or resampled buffers for UI rendering. Buffer behavior is polymorphic --
properties and path generation adapt to the source processor type (FFT, oscilloscope,
goniometer, envelope) via the internal PropertyObject system.

Complexity tiers:
  1. Basic visualization: getDisplayBuffer, setRingBufferProperties, createPath in a
     timer. Single oscilloscope, FFT overlay, or gain reduction meter.
  2. Multi-buffer management: + setActive. Multiple display buffers from different
     processors, disable invisible buffers to save CPU.
  3. Direct buffer access: + getReadBuffer, copyReadBuffer, getResizedBuffer. Custom
     processing of buffer data when createPath is not flexible enough.

Practical defaults:
  - Use a timer interval of 30ms (startTimer(30)) for smooth display updates --
    approximately 33 fps, sufficient for audio visualization without excessive CPU.
  - For FFT spectrum analysis: "BufferLength": 4096, "WindowType": "Flat Top",
    "UseDecibelScale": true, "UseLogarithmicFreqAxis": true.
  - Use setActive(false) on display buffers that are not currently visible. FFT
    analysis consumes CPU even when the result is not rendered.
  - For gain reduction / peak meters, use createPath() with sourceRange
    [0.0, 1.0, 0, -1] (normalised 0-1 range, full buffer length).

Common mistakes:
  - Modifying the buffer returned by getReadBuffer() -- it is a direct memory
    reference to the ring buffer internals. Use copyReadBuffer() for a safe copy.
  - Passing a target buffer to copyReadBuffer() whose size does not match the ring
    buffer sample count exactly -- causes silent failure. Check size first or use
    getResizedBuffer() for resampling.
  - Leaving all display buffers active regardless of which UI page is shown -- FFT
    runs on the audio thread even when not rendered. Disable invisible buffers with
    setActive(false).
  - Calling createPath() in the paint routine -- it acquires a lock on the ring
    buffer. Call it in the timer callback, store the path, then draw in paint.

Example:
  // Get a display buffer from an analyser module
  const var src = Synth.getDisplayBufferSource("Analyser1");
  const var db = src.getDisplayBuffer(0);

  // Configure FFT properties
  db.setRingBufferProperties({
      "BufferLength": 8192,
      "WindowType": "BlackmanHarris",
      "UseDecibelScale": true
  });

Methods (8):
  copyReadBuffer          createPath
  fromBase64              getReadBuffer
  getResizedBuffer        setActive
  setRingBufferProperties toBase64
