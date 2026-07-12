MidiPlayer::convertEventListToNoteRectangles(Array eventList, Array targetBounds) -> Array

Thread safety: UNSAFE -- Creates a temporary HiseMidiSequence and performs heap allocations for rectangle construction.
Converts an array of MessageHolder objects into note rectangles scaled to targetBounds. Unlike getNoteRectangleList() which reads the current sequence, this operates on an arbitrary event list -- useful for previewing edits before flushing. Each rectangle is [x, y, width, height] where x/width are normalised time position/duration and y/height are note number (127 at top, 1/128 height per note).
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var rects = mp.convertEventListToNoteRectangles(events, [0, 0, 500, 200]);
```
Pair with: MidiPlayer.getNoteRectangleList -- reads from current sequence instead of arbitrary list. MidiPlayer.getEventList -- provides the event list input.
Source:
  ScriptingApiObjects.cpp:6250  convertEventListToNoteRectangles() -> new HiseMidiSequence() -> getRectangleList(targetBounds)
