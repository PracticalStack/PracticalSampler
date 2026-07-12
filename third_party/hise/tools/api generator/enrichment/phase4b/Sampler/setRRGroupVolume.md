Sampler::setRRGroupVolume(Integer groupIndex, Integer gainInDecibels) -> undefined

Thread safety: SAFE
Sets the volume for a round robin group in decibels. Pass -1 as group index to
set the volume for the currently active group. The dB value is converted to
linear gain internally.
Pair with:
  setActiveGroup -- select which group is active
  getNumActiveGroups -- get group count
Source:
  ScriptingApi.cpp  Sampler::setRRGroupVolume()
    -> dB to linear conversion internally
