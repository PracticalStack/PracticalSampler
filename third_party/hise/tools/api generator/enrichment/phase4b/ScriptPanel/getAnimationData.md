ScriptPanel::getAnimationData() -> JSON

Thread safety: UNSAFE -- allocates JSON object
Returns a JSON object describing the current Lottie animation state. Requires
HISE_INCLUDE_RLOTTIE. Returns {active: false} if no animation is loaded.
Returned object properties: active (bool), currentFrame (int), numFrames (int),
frameRate (int).
Pair with:
  setAnimation -- load a Lottie animation
  setAnimationFrame -- render a specific frame
Source:
  ScriptingApiContent.cpp  ScriptPanel::getAnimationData()
