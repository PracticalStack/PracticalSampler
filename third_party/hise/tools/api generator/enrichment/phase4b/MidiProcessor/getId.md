MidiProcessor::getId() -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the module ID of the underlying MIDI processor (e.g., "Arpeggiator1").
Source:
  ScriptingApiObjects.cpp:4627  getId() -> mp->getId()
