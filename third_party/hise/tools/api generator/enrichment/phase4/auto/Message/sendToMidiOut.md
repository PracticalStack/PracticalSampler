Forwards the current event to the plugin's MIDI output. The event is automatically converted to artificial if it is not already.

> [!Warning:Requires EnableMidiOut project setting] The `EnableMidiOut` project setting must be enabled for MIDI output to function. In the HISE IDE, a missing setting produces a clear error. In exported plugins, no check occurs and the method silently does nothing if MIDI output was not enabled at export time.
