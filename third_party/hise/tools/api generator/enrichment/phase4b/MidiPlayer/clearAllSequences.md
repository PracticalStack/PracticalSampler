MidiPlayer::clearAllSequences() -> undefined

Thread safety: UNSAFE -- Calls clearSequences with sendNotificationAsync, which modifies the sequence list and triggers listener notifications.
Removes all loaded MIDI sequences and tracks from this player. Sends an async notification to all sequence listeners (triggering the sequence callback if set).
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.clearAllSequences();
```
Pair with: MidiPlayer.setFile, MidiPlayer.create -- clearing before loading/creating new sequences.
Source:
  ScriptingApiObjects.cpp:6250  clearAllSequences() -> MidiPlayer::clearSequences(sendNotificationAsync)
