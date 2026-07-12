DisplayBufferSource (object)
Obtain via: Synth.getDisplayBufferSource(processorId)

Handle to a processor's display buffer slots for real-time visualization.
Lightweight wrapper that locates a processor by ID and provides indexed
access to its DisplayBuffer objects (ring buffers for waveform/spectrum data).

Complexity tiers:
  1. Single buffer visualization: getDisplayBuffer. Chain
     getDisplayBufferSource(id).getDisplayBuffer(0) directly for simple
     gain reduction meters or FFT displays.
  2. Multi-buffer dynamics display: getDisplayBuffer with multiple indices
     from the same source. Store the DisplayBufferSource, call
     getDisplayBuffer() per index. Pairs with DisplayBuffer.getReadBuffer().
  3. Batch multi-channel setup: Loop over processor IDs, create a
     DisplayBufferSource per channel, collect DisplayBuffer refs into an
     array for switchable visualization.

Practical defaults:
  - Chain getDisplayBufferSource(id).getDisplayBuffer(0) directly when you
    only need one buffer -- no need to store the intermediate object.
  - Store the DisplayBufferSource when you need multiple buffer indices
    from the same processor.
  - Dynamics processors typically expose gain reduction at index 0 and
    peak at index 1.
  - 30ms timer interval is a good default for dynamics meters and spectrum
    displays.

Common mistakes:
  - Creating DisplayBufferSource inside a timer or control callback --
    Synth.getDisplayBufferSource() is onInit-only, throws error outside init.
  - Passing an out-of-range index to getDisplayBuffer() -- causes a script
    error. Check the processor's buffer count.
  - Chaining .getDisplayBuffer(0).getDisplayBuffer(1) -- getDisplayBuffer
    returns a DisplayBuffer, not another DisplayBufferSource. Call it on the
    source for each index.
  - Forgetting setRingBufferProperties() for FFT display -- buffer uses
    defaults that may not match your frequency range or resolution.

Example:
  // Get a display buffer source from a processor and access its first buffer
  const var dbs = Synth.getDisplayBufferSource("MyProcessor");
  const var db = dbs.getDisplayBuffer(0);

Methods (1):
  getDisplayBuffer
