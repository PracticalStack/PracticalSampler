ScriptTable::registerAtParent(Integer index) -> ScriptObject

Thread safety: UNSAFE
Registers this component's owned table at the parent processor external-data slot.
Returns a ScriptTableData handle for the registered slot.

Required setup:
  const var st = Content.addTable("EnvCurve", 20, 20);

Dispatch/mechanics:
  Delegates to registerComplexDataObjectAtParent(Table, index).
  Requires ProcessorWithDynamicExternalData -> registerExternalObject(Table, index, ownedObject).

Pair with:
  referToData -- bind other tables/components to the registered shared data

Anti-patterns:
  - Do NOT call repeatedly in playback callbacks -- register once in init and reuse the handle.
  - Do NOT assume every parent supports dynamic external data -- unsupported parents return undefined silently.

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:3350  ScriptTable::registerAtParent() -> registerComplexDataObjectAtParent()
  HISE/hi_core/hi_dsp/ProcessorInterfaces.cpp:449  ProcessorWithDynamicExternalData::registerExternalObject()
