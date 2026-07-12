MidiPlayer::setRecordEventCallback(Function recordEventCallback) -> undefined

Thread safety: UNSAFE -- Creates ScriptEventRecordProcessor, registers with player.
Registers an inline function that processes every MIDI event about to be recorded. Runs on the audio thread. You can modify the event (quantize, filter) -- changes are applied to the recorded data. Must be an inline function (realtime-safe); regular functions throw a script error.
Callback signature: f(MessageHolder event)
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
inline function onRecordEvent(event) { /* modify event */ }
mp.setRecordEventCallback(onRecordEvent);
```
Dispatch/mechanics: Creates ScriptEventRecordProcessor that registers as MidiPlayer::EventRecordProcessor. On each recorded event, processRecordedEvent() wraps the HiseEvent in a ScriptingMessageHolder, calls callSync() on the audio thread, then reads back the (possibly modified) event.
Pair with: MidiPlayer.record -- starts recording that triggers this callback.
Anti-patterns: Passing a non-inline function throws a script error. The callback must be realtime-safe since it runs on the audio thread.
Source:
  ScriptingApiObjects.cpp:6250  setRecordEventCallback() -> new ScriptEventRecordProcessor() -> MidiPlayer::addEventRecordProcessor() -> processRecordedEvent() -> callSync()
