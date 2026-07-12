## getValue

**Examples:**

```javascript:read-note-state-in-paint-routine
// Title: Read per-note state inside a paint routine
// Context: A visual keyboard display reads the MidiList to highlight
// which keys are currently held. The MidiList is updated in MIDI
// callbacks and read during UI rendering.

const var noteStates = Engine.createMidiList();
noteStates.fill(0);

const var keyboard = Content.getComponent("KeyboardPanel");

keyboard.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    for (i = 0; i < 128; i++)
    {
        if (noteStates.getValue(i) > 0)
        {
            // Draw a highlight for each active note
            local x = i * 4;
            g.setColour(0xFFFF8800);
            g.fillRect([x, 0, 3, this.getHeight()]);
        }
    }
});
```
```json:testMetadata:read-note-state-in-paint-routine
{
  "testable": false,
  "skipReason": "Requires an existing ScriptPanel named KeyboardPanel and visual rendering to verify the paint routine output"
}
```
