SliderPackProcessor::getSliderPack(int sliderPackIndex) -> ScriptObject

Thread safety: UNSAFE -- creates a new ScriptSliderPackData object on the heap
Creates a data reference to the slider pack at the given zero-based index.
Returns a SliderPackData handle for reading/writing individual slider values.
Required setup:
  const var spp = Synth.getSliderPackProcessor("ArrayModulator1");
Dispatch/mechanics:
  checkValidObject() -> dynamic_cast<ProcessorWithExternalData*>(sp)
    -> new ScriptSliderPackData(scriptProcessor, sliderPackIndex, externalDataHolder)
  No bounds checking at this level -- handled inside ScriptSliderPackData
Pair with:
  Synth.getSliderPackProcessor -- factory method that creates the wrapper (onInit only)
  ScriptSliderPackData (SliderPackData) -- returned handle class with setValue/getValue/setNumSliders
Source:
  ScriptingApiObjects.cpp:5358  ScriptSliderPackProcessor::getSliderPack()
    -> dynamic_cast<ProcessorWithExternalData*>(sp.get())
    -> new ScriptSliderPackData(getScriptProcessor(), sliderPackIndex, ed)
