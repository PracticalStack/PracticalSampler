## ignoreEvent

**Examples:**

```javascript:keyswitch-articulation-switching
// Title: Keyswitch-based articulation switching
// Context: Sampler instruments use low notes as keyswitches to select
// articulations. The keyswitch note is consumed (ignored) and triggers
// a MidiMuter bypass change instead of producing sound.

const var KEYSWITCH_LOW = 24;  // C1
const var KEYSWITCH_HIGH = 35; // B1

const var muters = [Synth.getMidiProcessor("SusMuter"),
                    Synth.getMidiProcessor("StacMuter"),
                    Synth.getMidiProcessor("TremMuter")];

function onNoteOn()
{
    local n = Message.getNoteNumber();

    if (n >= KEYSWITCH_LOW && n <= KEYSWITCH_HIGH)
    {
        Message.ignoreEvent(true);

        local artIndex = n - KEYSWITCH_LOW;

        // Enable one articulation, mute all others
        for (i = 0; i < muters.length; i++)
            muters[i].setAttribute(0, i == artIndex ? 0 : 1);
    }
}
```
```json:testMetadata:keyswitch-articulation-switching
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:note-range-filter
// Title: Note range filter
// Context: A MIDI processor that only passes through a single note number,
// ignoring everything else. Useful for isolating individual drum pads
// in a multi-pad instrument.

const var noteFilter = Content.addKnob("TargetNote", 0, 0);
noteFilter.setRange(0, 127, 1);

function onNoteOn()
{
    // ignoreEvent accepts a boolean-like argument:
    // true = ignore, false = allow through
    Message.ignoreEvent(Message.getNoteNumber() != noteFilter.getValue());
}

function onNoteOff()
{
    Message.ignoreEvent(Message.getNoteNumber() != noteFilter.getValue());
}
```
```json:testMetadata:note-range-filter
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:monophonic-ignore-resynthesize
// Title: Monophonic mode using ignore-and-resynthesize
// Context: Suppress the original note and create a new artificial one
// so the script has full control over voice management and glide.

reg lastEventId = -1;
reg lastNote = -1;

const var glideKnob = Content.addKnob("Glide", 10, 10);
glideKnob.setRange(0, 1, 0.01);

function onNoteOn()
{
    Message.ignoreEvent(true);

    local newId = Synth.addNoteOn(Message.getChannel(),
                                   Message.getNoteNumber(),
                                   Message.getVelocity(), 0);

    if (lastNote != -1)
    {
        Synth.noteOffByEventId(lastEventId);

        // Glide from previous note
        local delta = (Message.getNoteNumber() - lastNote) *
                      Math.min(1.0, glideKnob.getValue());

        Synth.addPitchFade(newId, 0, -delta, 0);
        Synth.addPitchFade(newId, parseInt(glideKnob.getValue() * 1024), 0, 0);
    }

    lastEventId = newId;
    lastNote = Message.getNoteNumber();
}

function onNoteOff()
{
    if (Message.getNoteNumber() == lastNote)
    {
        Message.ignoreEvent(true);
        Synth.noteOffByEventId(lastEventId);
        lastNote = -1;
    }
}
```
```json:testMetadata:monophonic-ignore-resynthesize
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```
