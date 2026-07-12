ChildSynth::getId() -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations
Returns the module ID string of the wrapped synth processor. Returns empty string
if the object is invalid.
Source:
  ScriptingApiObjects.cpp  getId()
    -> synth->getId()
