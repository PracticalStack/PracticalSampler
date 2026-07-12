Registers an inline function that processes every MIDI event about to be recorded. The callback receives a single MessageHolder argument and can modify or filter events before they are written to the sequence - for example, quantising timestamps to a grid or ignoring notes outside a target range.

> [!Warning:Runs on audio thread, use inline function] This callback runs on the audio thread and must be an inline function. Regular functions will throw a script error. Avoid any operations that allocate memory or block.
