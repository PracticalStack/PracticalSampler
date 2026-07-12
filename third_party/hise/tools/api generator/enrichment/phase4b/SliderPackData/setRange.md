SliderPackData::setRange(Double minValue, Double maxValue, Double stepSize) -> undefined

Thread safety: SAFE -- sets two member variables with no locks or allocations.
Sets the value range and step size for all sliders. The range constrains values in the
UI (ScriptSliderPack component) and step size controls quantization. Default range is
[0.0, 1.0] with step size 0.01. Does not clamp existing values.
Anti-patterns:
  - Do NOT expect setRange to clamp existing values -- only new values entered
    through the UI are constrained
Source:
  SliderPack.cpp  SliderPackData::setRange()
    -> sets sliderRange and stepSize member variables
