DisplayBuffer::setActive(Integer shouldBeActive) -> undefined

Thread safety: SAFE
Enables or disables the ring buffer. When disabled, the DSP writer skips writing to
the ring buffer, reducing CPU overhead for display buffers not currently visible.
Pair with:
  setRingBufferProperties -- configure buffer before activating
Anti-patterns:
  - Do NOT leave all display buffers active when only one page/tab is visible --
    FFT analysis runs on the audio thread even when the UI is not rendering the
    result. Tie activation to UI visibility.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::setActive()
    -> SimpleRingBuffer::setActive(shouldBeActive)
    -> sets internal bool; DSP writer checks before write
