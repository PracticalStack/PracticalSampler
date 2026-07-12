## setType

**Examples:**

```javascript:step-sequencer-from-sliderpack
// Title: Building a step sequencer pattern from slider pack data
// Context: A step sequencer reads velocity values from a slider pack and
// constructs a complete MIDI sequence as NoteOn/NoteOff pairs, then flushes
// the list to a MidiPlayer for playback.

const var mp = Synth.getMidiPlayer("Player1");
const var sliderPack = Content.getComponent("StepSliderPack");

mp.setUseTimestampInTicks(true);

const var SIXTEENTH_TICKS = 120; // Ticks per sixteenth note (at 480 PPQN)

inline function rebuildSequence()
{
    local list = [];
    list.reserve(sliderPack.getNumSliders() * 2);

    for (i = 0; i < sliderPack.getNumSliders(); i++)
    {
        local vel = sliderPack.getSliderValueAt(i);

        if (vel > 0.0)
        {
            local on = Engine.createMessageHolder();
            local off = Engine.createMessageHolder();

            on.setType(on.NoteOn);
            off.setType(on.NoteOff);

            on.setChannel(1);
            off.setChannel(1);

            on.setNoteNumber(64);
            off.setNoteNumber(64);

            on.setVelocity(parseInt(vel * 127));

            on.setTimestamp(i * SIXTEENTH_TICKS);
            off.setTimestamp(i * SIXTEENTH_TICKS + parseInt(0.99 * SIXTEENTH_TICKS));

            list.push(on);
            list.push(off);
        }
    }

    mp.flushMessageList(list);
}
```
```json:testMetadata:step-sequencer-from-sliderpack
{
  "testable": false,
  "skipReason": "Requires MidiPlayer module (Synth.getMidiPlayer) and Content.getComponent() for SliderPack"
}
```

Each event requires a fresh `Engine.createMessageHolder()` call. Pushing the same holder reference into the array multiple times creates aliases - all entries would reflect the last modification.
