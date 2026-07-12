Estimates the fundamental frequency in Hz for a selected region of the buffer. Pass a valid sample rate first, then optionally limit analysis with `startSample` and `numSamples`.

> [!Warning:Requires pitch detection build flag] This method is only present in builds where pitch detection support is enabled. In other builds, `Buffer.detectPitch` is unavailable.
