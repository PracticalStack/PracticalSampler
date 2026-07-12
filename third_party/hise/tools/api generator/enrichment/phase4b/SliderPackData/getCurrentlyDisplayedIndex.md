SliderPackData::getCurrentlyDisplayedIndex() -> Double

Thread safety: SAFE
Returns the last display index value from the internal updater. Reflects the
ruler/playback indicator position in a ScriptSliderPack UI component.
Returns 0.0 if no display index has been set.
Pair with:
  setDisplayCallback -- observe display index changes reactively
Source:
  ScriptingApiObjects.cpp  ScriptComplexDataReferenceBase::getCurrentDisplayIndexBase()
    -> returns cached display value from ComplexDataUIUpdaterBase
