ScriptSliderPack::registerAtParent(Integer index) -> Object

Thread safety: UNSAFE
Registers the owned slider-pack data object in the parent processor dynamic external-data pool and returns a ScriptSliderPackData handle on success.

Dispatch/mechanics:
  registerComplexDataObjectAtParent(index) requires ProcessorWithDynamicExternalData.
  For SliderPack type it returns new ScriptSliderPackData(getScriptProcessor(), index).

Pair with:
  referToData -- bind this or other UI components to the returned handle
  set("SliderPackIndex", ...) -- select external slot when using processor-connected mode

Anti-patterns:
  - Do NOT assume registration always succeeds -- unsupported parents can return undefined without an error.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::registerAtParent() -> ComplexDataScriptComponent::registerComplexDataObjectAtParent()
