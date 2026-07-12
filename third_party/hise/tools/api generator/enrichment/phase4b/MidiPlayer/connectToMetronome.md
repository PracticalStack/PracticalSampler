MidiPlayer::connectToMetronome(String metronome) -> undefined

Thread safety: UNSAFE -- Processor lookup traverses the module tree.
Connects this MIDI player to a MidiMetronome effect module by its processor ID. Once connected, the metronome follows the player's transport state and position.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.connectToMetronome("Metronome1");
```
Anti-patterns: If the given ID does not match a MidiMetronome processor, a script error is thrown. No partial matching or type-agnostic lookup is performed.
Source:
  ScriptingApiObjects.cpp:6250  connectToMetronome() -> ProcessorHelpers::getFirstProcessorWithName() -> MidiMetronome::connectToPlayer()
