ScriptAudioWaveform::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] representing local bounds reduced by the given amount.
The local bounds start at [0, 0, width, height].

Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds()
    -> [0, 0, width, height] reduced by reduceAmount on each edge
