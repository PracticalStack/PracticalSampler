Injects a pre-constructed MIDI event from a `MessageHolder` object into the processing buffer. The event is marked as artificial regardless of its original state. The return value depends on the event type: note-on returns the assigned event ID, note-off returns the timestamp, and all other event types return 0.

> [!Warning:Note-off needs matching registered note-on] For note-off events, the method matches the corresponding note-on by channel and note number. If no matching note-on was registered (e.g. from a different source), the note-off may fail to stop the intended voice.
