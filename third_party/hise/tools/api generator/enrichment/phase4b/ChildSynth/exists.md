ChildSynth::exists() -> int

Thread safety: SAFE
Returns true if the wrapped synth module still exists and has not been deleted.
Checks both objectExists() (non-null) and objectDeleted() (weak reference validity).
Does not throw a script error on invalid objects -- designed for safe validity testing.
Source:
  ConstScriptingObject::checkValidObject()
    -> objectExists() checks synth != nullptr
    -> objectDeleted() checks synth.get() == nullptr (weak ref)
