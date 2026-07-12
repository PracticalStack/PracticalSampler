ScriptSliderPack::setUsePreallocatedLength(Integer numMaxSliders) -> undefined

Thread safety: UNSAFE
Enables fixed-capacity preallocation for SliderPackData storage; 0 disables preallocation.

Dispatch/mechanics:
  SliderPackData switches backing storage to preallocated buffer when enabled.
  setNumSliders and swapBuffer reuse that buffer up to configured capacity.

Pair with:
  set("sliderAmount", ...) -- effective count must stay within preallocated cap
  getNumSliders -- validate active slider count after resizing

Anti-patterns:
  - Do NOT exceed numMaxSliders while preallocation is active -- effective slider count is capped.

Source:
  ScriptingApiContent.cpp:3498  ScriptSliderPack::setUsePreallocatedLength() -> SliderPackData preallocation mode
