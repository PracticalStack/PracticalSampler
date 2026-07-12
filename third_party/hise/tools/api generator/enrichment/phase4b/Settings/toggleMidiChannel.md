Settings::toggleMidiChannel(Integer index, Integer value) -> undefined

Thread safety: UNSAFE -- accesses the main synth chain's channel filter data
Enables or disables a MIDI channel for the main synth chain. Index 0 toggles all
channels at once. Indices 1-16 target individual MIDI channels (1-based, matching
standard MIDI channel numbering).

Dispatch/mechanics:
  mc->getMainSynthChain()->getActiveChannelData()
    -> index 0: setEnableAllChannels(value)
    -> index 1-16: setEnableMidiChannel(index - 1, value) (converted to 0-based)

Anti-patterns:
  - Do NOT use index 1 expecting "all channels" -- index 0 controls all channels,
    indices 1-16 are individual MIDI channels

Pair with:
  isMidiChannelEnabled -- check channel state

Source:
  ScriptingApi.cpp  Settings::toggleMidiChannel()
    -> HiseEvent::ChannelFilterData::setEnableAllChannels / setEnableMidiChannel
