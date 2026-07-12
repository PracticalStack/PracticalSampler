ScriptPanel::setAnimation(String base64LottieAnimation) -> undefined

Thread safety: UNSAFE -- allocates animation resources
Loads a Lottie animation from a base64-encoded JSON string. Requires
HISE_INCLUDE_RLOTTIE. The animation is sized to match the panel dimensions
with a 2x scale factor.
Pair with:
  setAnimationFrame -- render a specific frame after loading
  getAnimationData -- query animation state (active, currentFrame, numFrames, frameRate)
Source:
  ScriptingApiContent.cpp  ScriptPanel::setAnimation()
