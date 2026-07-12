Copies the current MIDI event into a `MessageHolder` object, allowing event data to persist beyond the callback scope. The Message object's internal pointer is only valid during callback execution, so `store()` is necessary whenever you need to reference event data later - for example, tracking held notes for release sample triggering or building note stacks.

The copy is a complete snapshot including note number, velocity, channel, transpose, gain, detune, start offset, event ID, and timestamp.
