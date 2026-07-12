Starts the internal master clock at the given sample timestamp offset within the current audio block. If called from within audio rendering, it immediately processes the grid and triggers transport callbacks for the current block. This is a global operation - it affects the shared MasterClock.

> [!Warning:Pass Message.getTimestamp for accuracy] When calling from a MIDI callback, always pass `Message.getTimestamp()` instead of 0. The timestamp provides sub-block accuracy - using 0 introduces up to one buffer's worth of timing jitter (e.g., ~5.8ms at 512 samples / 44.1kHz).
