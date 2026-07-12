Adds an artificial note-off event to the MIDI buffer with an explicit channel and sample-accurate timestamp. The note-off is matched to a note-on by channel and note number, not by event ID. For reliable voice management with overlapping notes on the same pitch, prefer `noteOffByEventId` or `noteOffDelayedByEventId`.

> [!Warning:Minimum timestamp is 1 sample] The timestamp is clamped to a minimum of 1 sample even when 0 is passed. Use `noteOffByEventId` for immediate note-offs at the current sample position.
