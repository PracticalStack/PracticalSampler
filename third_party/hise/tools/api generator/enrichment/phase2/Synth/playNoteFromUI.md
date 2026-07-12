## playNoteFromUI

**Examples:**

```javascript:custom-keyboard-panel
// Title: Custom on-screen keyboard built with a ScriptPanel
// Context: A ScriptPanel handles mouse clicks to simulate a keyboard. playNoteFromUI
// routes notes through the real MIDI input pipeline so isKeyDown() and the on-screen
// keyboard floating tile both reflect the correct state. noteOffFromUI is used for release.

const var keyboard = Content.getComponent("KeyboardPanel");
keyboard.set("allowCallbacks", "Clicks & Hover");

keyboard.data.currentNote = -1;

keyboard.setMouseCallback(function(event)
{
    // Release on mouse leave
    if (!event.hover && this.data.currentNote != -1)
    {
        Synth.noteOffFromUI(1, this.data.currentNote);
        this.data.currentNote = -1;
    }

    // Convert mouse position to note number (simplified)
    local noteNumber = 24 + parseInt(event.x / 15.0);

    if (event.clicked)
    {
        Synth.playNoteFromUI(1, noteNumber, 127);
        this.data.currentNote = noteNumber;
    }

    if (event.mouseUp && this.data.currentNote != -1)
    {
        Synth.noteOffFromUI(1, this.data.currentNote);
        this.data.currentNote = -1;
    }
});
```
```json:testMetadata:custom-keyboard-panel
{
  "testable": false,
  "skipReason": "Requires a pre-existing KeyboardPanel component and mouse interaction to trigger the callback"
}
```

**Pitfalls:**
- Always pair `playNoteFromUI` with `noteOffFromUI`, never with `noteOffByEventId`. Notes injected through the UI pipeline are real (non-artificial) events with no event ID tracking. Mixing `playNoteFromUI` with `noteOffByEventId` causes stuck notes.
