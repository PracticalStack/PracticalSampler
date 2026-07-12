Sets the sample start offset on the current event. The sound generator skips ahead by this many samples when the voice starts, which is useful for starting playback at a specific position within a sample or wavetable cycle. The maximum offset is 65535 samples (approximately 1.36 seconds at 48kHz).

> [!Warning:Controls playback position, not timing] Start offset controls where playback begins within the sample. It does not delay when the event is processed - use `Message.delayEvent()` for timing delays.
