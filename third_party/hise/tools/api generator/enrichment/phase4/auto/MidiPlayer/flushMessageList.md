Writes the given array of MessageHolder objects into the current sequence's current track, replacing the existing MIDI data. This operation is undoable if the undo manager has been enabled.

> [!Warning:Every note-on needs matching note-off] Every note-on must have a matching note-off in the list. Unpaired note-ons will sustain indefinitely during playback. Make sure the timestamp mode matches how the events were read - if you used `setUseTimestampInTicks(true)` when calling `getEventList()`, keep it enabled when flushing.
