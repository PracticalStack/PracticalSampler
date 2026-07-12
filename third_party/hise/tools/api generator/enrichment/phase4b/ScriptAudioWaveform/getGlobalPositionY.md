ScriptAudioWaveform::getGlobalPositionY() -> Integer

Thread safety: SAFE
Returns the absolute y-position relative to the interface root, computed by
recursively adding parent component y-offsets.

Pair with:
  getGlobalPositionX -- for the horizontal position

Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionY()
    -> recursive parent y-offset accumulation
