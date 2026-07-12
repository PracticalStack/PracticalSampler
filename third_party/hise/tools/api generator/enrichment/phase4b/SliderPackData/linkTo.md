SliderPackData::linkTo(var other) -> undefined

Thread safety: UNSAFE -- modifies event listener registrations and re-resolves the internal data pointer.
Links this SliderPackData to another so they share the same underlying data buffer.
After linking, changes through either handle affect the shared data.
Required setup:
  const var spd1 = Engine.createAndRegisterSliderPackData(0);
  const var spd2 = Engine.createAndRegisterSliderPackData(1);
Dispatch/mechanics:
  linkToInternal() -> validates type match (must both be SliderPack)
  -> ExternalDataHolder::linkTo(type, src, srcIndex, dstIndex)
  -> re-registers as EventListener on the new complex object
Anti-patterns:
  - Do NOT pass a Table, AudioFile, or other complex data type -- causes
    "Type mismatch" script error. Must be another SliderPackData.
Source:
  ScriptingApiObjects.cpp:1560  ScriptComplexDataReferenceBase::linkToInternal()
    -> validates ConstScriptingObject type match
    -> pdst->linkTo(type, *psrc, other->index, index)
