## getControllerValue

**Examples:**

```javascript:sustain-pedal-threshold
// Title: Sustain pedal handling with correct threshold
// Context: CC64 (sustain pedal) is the most commonly handled controller.
// Values above 64 mean "pedal down" per MIDI convention.

reg pedalDown = false;

function onController()
{
    if (Message.getControllerNumber() == 64)
    {
        local wasDown = pedalDown;
        pedalDown = Message.getControllerValue() > 64;

        if (wasDown && !pedalDown)
        {
            // Pedal released -- trigger release samples for held notes
        }
    }
}
```
```json:testMetadata:sustain-pedal-threshold
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:cc-driven-note-muting
// Title: CC-driven randomized note muting
// Context: An internal CC (sent from a sequencer grid) controls the
// probability of muting the next note. The controller value is
// normalized to 0.0-1.0 and compared against a random threshold.

reg muteNextNote = false;

function onController()
{
    if (Message.getControllerNumber() == 99)
    {
        // Normalize CC value (0-127) to probability
        local probability = Message.getControllerValue() / 127.0;
        muteNextNote = probability > Math.random();
    }
}

function onNoteOn()
{
    if (muteNextNote)
    {
        Message.ignoreEvent(true);
        muteNextNote = false;
    }
}
```
```json:testMetadata:cc-driven-note-muting
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```
