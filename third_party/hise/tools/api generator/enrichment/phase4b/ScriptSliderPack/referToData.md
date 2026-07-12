ScriptSliderPack::referToData(Object sliderPackData) -> undefined

Thread safety: UNSAFE
Rebinds this component to another slider-pack data source, compatible complex-data component, or -1 to restore internal data.

Dispatch/mechanics:
  referToDataBase(var) accepts ScriptComplexDataReferenceBase, ComplexDataScriptComponent, or -1.
  getUsedData(SliderPack) then resolves referred holder before processor slot or owned object.

Pair with:
  registerAtParent -- obtain a ScriptSliderPackData handle from the parent slot system
  getNumSliders -- confirm the new source has expected lane count

Anti-patterns:
  - Do NOT pass unsupported argument types -- current behavior can keep the old binding silently.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::referToData() -> ComplexDataScriptComponent::referToDataBase()
