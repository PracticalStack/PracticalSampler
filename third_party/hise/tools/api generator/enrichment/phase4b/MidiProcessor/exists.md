MidiProcessor::exists() -> Integer

Thread safety: SAFE
Returns whether the underlying MIDI processor module reference is still valid.
Returns false and prints a console error if the module was deleted or the ID was invalid.
Source:
  ScriptingApiObjects.h:2349  objectExists() -> checks WeakReference<MidiProcessor> != nullptr
