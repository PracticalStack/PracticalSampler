ScriptAudioWaveform::getGlobalPositionX() -> Integer

Thread safety: SAFE
Returns the absolute x-position relative to the interface root, computed by
recursively adding parent component x-offsets.

Pair with:
  getGlobalPositionY -- for the vertical position

Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionX()
    -> recursive parent x-offset accumulation
