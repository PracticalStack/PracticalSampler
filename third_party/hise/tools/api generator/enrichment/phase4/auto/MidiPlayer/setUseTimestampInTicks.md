Switches the timestamp format used by event list operations between audio samples (default) and MIDI ticks (960 per quarter note). Tick mode is recommended for musical editing because tick values are tempo-independent, making grid alignment and quantisation straightforward.

> [!Warning:Must match mode for read and write] This setting affects both reading (`getEventList()`) and writing (`flushMessageList()`). If you read events in tick mode, you must also flush in tick mode, or the timestamps will be misinterpreted.
