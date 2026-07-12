MidiPlayer::setSequenceCallback(Function updateFunction) -> undefined

Thread safety: UNSAFE -- Creates WeakCallbackHolder, stores callback reference.
Registers a callback that fires whenever the MIDI sequence data changes (load, edit, clear). The callback fires asynchronously on the message thread. Immediately fires once upon registration for UI state initialization.
Callback signature: f(MidiPlayer midiPlayer)
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
inline function onSequenceChange(player) { /* update UI */ }
mp.setSequenceCallback(onSequenceChange);
```
Dispatch/mechanics: Stores callback in WeakCallbackHolder. On sequence changes (sequenceLoaded/sequencesCleared), callUpdateCallback() invokes the callback via .call() (async/deferred, not callSync). Argument is the MidiPlayer object itself.
Pair with: MidiPlayer.setPlaybackCallback -- for transport state changes. MidiPlayer.connectToPanel -- alternative auto-repaint approach.
Source:
  ScriptingApiObjects.cpp:6250  setSequenceCallback() -> WeakCallbackHolder -> callUpdateCallback() -> .call()
