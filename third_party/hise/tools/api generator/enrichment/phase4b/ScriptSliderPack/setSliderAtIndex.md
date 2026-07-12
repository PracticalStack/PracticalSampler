ScriptSliderPack::setSliderAtIndex(Integer index, Double value) -> undefined

Thread safety: UNSAFE
Sets one slider value in the currently bound SliderPackData source.

Dispatch/mechanics:
  Resolves active data source via ComplexDataScriptComponent routing.
  Notification and callback fanout depend on allValueChangeCausesCallback.

Pair with:
  getSliderValueAt -- read back one indexed value
  setAllValues -- bulk write alternative
  setAllValueChangeCausesCallback -- callback fanout policy

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::setSliderAtIndex() indexed write path
