MidiPlayer::setAutomationHandlerConsumesControllerEvents(Integer shouldBeEnabled) -> undefined

Thread safety: SAFE
When enabled, CC messages played back from the MIDI file are sent to the global MIDI automation handler. The handler can consume these events, preventing them from passing through the signal chain -- effectively allowing MIDI file playback to drive automated parameters.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setAutomationHandlerConsumesControllerEvents(true);
```
Source:
  ScriptingApiObjects.cpp:6250  setAutomationHandlerConsumesControllerEvents() -> sets globalMidiHandlerConsumesCC on MidiPlayer core
