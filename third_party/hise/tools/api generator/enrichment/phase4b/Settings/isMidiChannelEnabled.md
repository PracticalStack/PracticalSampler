Settings::isMidiChannelEnabled(Integer index) -> Integer

Thread safety: UNSAFE -- accesses the main synth chain's channel filter data
Returns whether a specific MIDI channel is enabled. Index 0 checks if all channels
are enabled. Indices 1-16 check individual MIDI channels (1-based).

Pair with:
  toggleMidiChannel -- enable/disable a channel

Source:
  ScriptingApi.cpp  Settings::isMidiChannelEnabled()
    -> mc->getMainSynthChain()->getActiveChannelData()
