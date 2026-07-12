## restoreState

**Examples:**

```javascript:midiplayer-pattern-restore
// Title: Restoring MidiPlayer state via asMidiProcessor() cast
// Context: MidiPlayer modules need generic state serialization for custom
// preset loading. Cast to MidiProcessor to access exportState/restoreState,
// then continue using the MidiPlayer handle for playback control.

const var NUM_PLAYERS = 4;
const var players = [];

for (i = 0; i < NUM_PLAYERS; i++)
    players.push(Synth.getMidiPlayer("MIDIPlayer" + (i + 1)));

inline function loadPatternData(patternObject)
{
    if (!isDefined(patternObject.MidiData))
        return;

    for (i = 0; i < NUM_PLAYERS; i++)
    {
        // Cast MidiPlayer to MidiProcessor for state restore
        local mp = players[i].asMidiProcessor();

        mp.restoreState(patternObject.MidiData[i]);
        mp.setAttribute(mp.PlaybackSpeed, patternObject.Rate[i]);

        // Back to MidiPlayer handle for playback operations
        players[i].setPlaybackPosition(0.0);
    }
}
```
```json:testMetadata:midiplayer-pattern-restore
{
  "testable": false,
  "skipReason": "Requires MidiPlayer modules with loaded MIDI data and pattern objects"
}
```

```javascript:effect-slot-default-fallback
// Title: Restoring effect state with default fallback
// Context: When loading an effect into a dynamic slot, restore its default
// state from a pre-captured base64 string. This ensures predictable initial
// parameter values regardless of previous slot contents.

const var slot = Synth.getSlotFX("FXSlot1");

// Pre-captured default states for each effect type
const var defaultStates = {
    "Delay": "base64-encoded-default-state...",
    "Reverb": "base64-encoded-default-state...",
    "Filter": "base64-encoded-default-state..."
};

inline function loadEffect(effectName)
{
    slot.setEffect(effectName);

    local fx = slot.getCurrentEffect();

    if (isDefined(defaultStates[effectName]))
        fx.restoreState(defaultStates[effectName]);
}
```
```json:testMetadata:effect-slot-default-fallback
{
  "testable": false,
  "skipReason": "Requires SlotFX module and valid pre-captured base64 state strings"
}
```
