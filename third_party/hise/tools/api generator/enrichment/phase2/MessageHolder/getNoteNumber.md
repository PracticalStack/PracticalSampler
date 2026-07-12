## getNoteNumber

**Examples:**

```javascript:note-tracking-per-key-uptime
// Title: Note tracking with per-key uptime for duration-based behavior
// Context: A release trigger script stores NoteOn events and records when
// each key was pressed. On NoteOff, it reads the stored event's note number
// to look up the press time and calculate hold duration.

const var holder = Engine.createMessageHolder();
const var uptimes = [];
uptimes.reserve(128);

for (i = 0; i < 128; i++)
    uptimes[i] = 0.0;

inline function handleNoteOn()
{
    Message.store(holder);
    // Use getNoteNumber() on the stored event to index into per-key arrays
    uptimes[holder.getNoteNumber()] = Engine.getUptime();
}

inline function handleNoteOff()
{
    Message.store(holder);

    // Calculate how long this specific note was held
    local duration = Engine.getUptime() - uptimes[holder.getNoteNumber()];

    if (duration < 0.8)
        Console.print("Short press: " + holder.getNoteNumber());
    else
        Console.print("Long press: " + holder.getNoteNumber());
}
```
```json:testMetadata:note-tracking-per-key-uptime
{
  "testable": false,
  "skipReason": "Requires MIDI callbacks (onNoteOn/onNoteOff) with Message.store() - cannot be triggered from onInit"
}
```
