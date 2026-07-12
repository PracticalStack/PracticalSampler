ScriptPanel::setAnimationFrame(Integer numFrame) -> undefined

Thread safety: UNSAFE -- renders frame, flushes draw handler
Renders the specified frame of the loaded Lottie animation. Requires
HISE_INCLUDE_RLOTTIE and a prior call to setAnimation().
Pair with:
  setAnimation -- load the animation first
  getAnimationData -- query numFrames to know valid range (0 to numFrames-1)
Source:
  ScriptingApiContent.cpp  ScriptPanel::setAnimationFrame()
