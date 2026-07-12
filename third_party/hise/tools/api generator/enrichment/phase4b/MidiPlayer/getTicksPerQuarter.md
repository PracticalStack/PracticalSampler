MidiPlayer::getTicksPerQuarter() -> Integer

Thread safety: SAFE -- Returns a compile-time constant (960).
Returns the MIDI tick resolution per quarter note. Always returns 960. Use to convert between tick timestamps and musical positions when setUseTimestampInTicks(true) is active.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var tpq = mp.getTicksPerQuarter();
```
Pair with: MidiPlayer.setUseTimestampInTicks -- enables tick-based timestamps that use this resolution.
Source:
  ScriptingApiObjects.cpp:6250  getTicksPerQuarter() -> HiseMidiSequence::TicksPerQuarter (constexpr 960)
