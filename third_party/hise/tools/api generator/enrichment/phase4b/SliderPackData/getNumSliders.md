SliderPackData::getNumSliders() -> Integer

Thread safety: SAFE -- acquires a lightweight read lock (atomic) to read the buffer size.
Returns the number of sliders in the pack. Default is 16 if not changed via setNumSliders().
Pair with:
  setNumSliders -- set the count this method reads
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::getNumSliders()
    -> SliderPackData::getNumSliders() -> reads dataBuffer->size under read lock
