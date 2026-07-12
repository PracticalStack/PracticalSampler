AudioFile::linkTo(ScriptObject otherAudioFile) -> undefined

Thread safety: UNSAFE -- modifies listener registrations and data holder linkage.
Links this AudioFile's data slot to another AudioFile's data source. After
linking, both references share the same underlying audio buffer. Changes to
one are reflected in the other.

Dispatch/mechanics:
  linkToInternal(o) -> validates type match
    -> holder->linkTo(type, sourceHolder, srcIndex, dstIndex)
    -> re-registers event listener on new complex object

Anti-patterns:
  - Do NOT link to a different data type (e.g. AudioFile to Table) -- throws
    "Type mismatch" script error.
  - Do NOT pass a non-data-reference object -- throws "Not a data object".

Source:
  ScriptingApiObjects.cpp:1560  ScriptComplexDataReferenceBase::linkToInternal()
    -> validates dynamic_cast<ScriptComplexDataReferenceBase*>
    -> validates type match
    -> holder->linkTo(type, *psrc, other->index, index)
    -> re-registers EventListener on new complexObject
