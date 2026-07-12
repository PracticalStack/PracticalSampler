## sort

**Examples:**

```javascript:sorted-held-keys-arpeggiator
// Title: Maintain a sorted held-keys array for an arpeggiator
// Context: An arpeggiator tracks held MIDI notes in two arrays:
// one in input order (for "as played" mode) and one always sorted
// (for "up/down" mode). Calling sort() after each push keeps the
// sorted copy in ascending pitch order.

const var heldKeys = [];       // insertion order
const var heldKeysSorted = []; // always sorted by pitch

inline function addKey(noteNumber)
{
    if(heldKeys.contains(noteNumber))
        return;

    heldKeys.push(noteNumber);
    heldKeysSorted.push(noteNumber);
    heldKeysSorted.sort();
}

inline function removeKey(noteNumber)
{
    heldKeys.remove(noteNumber);
    heldKeysSorted.remove(noteNumber);
}

addKey(60);
addKey(48);
addKey(72);
Console.print(trace(heldKeys));       // [60, 48, 72]
Console.print(trace(heldKeysSorted)); // [48, 60, 72]
```
```json:testMetadata:sorted-held-keys-arpeggiator
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["[60, 48, 72]", "[48, 60, 72]"]}
  ]
}
```
