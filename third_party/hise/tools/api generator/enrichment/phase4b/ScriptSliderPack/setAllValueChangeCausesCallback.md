ScriptSliderPack::setAllValueChangeCausesCallback(Integer shouldBeEnabled) -> undefined

Thread safety: SAFE
Enables or disables control-callback triggering for non-undo bulk and indexed write helpers.

Dispatch/mechanics:
  Toggles internal allValueChangeCausesCallback.
  setSliderAtIndex and setAllValues read this flag to choose notify path vs display-only update.

Pair with:
  setAllValues -- callback behavior switch applies here
  setSliderAtIndex -- callback behavior switch applies here
  setAllValuesWithUndo -- related path, but currently still notifies

Source:
  ScriptingApiContent.h:1498  ScriptSliderPack::allValueChangeCausesCallback state and API method
