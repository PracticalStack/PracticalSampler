MidiPlayer::getNoteRectangleList(Array targetBounds) -> Array

Thread safety: UNSAFE -- Iterates sequence events and constructs rectangle objects on the heap.
Returns an array of note rectangles for all notes in the current sequence, scaled to targetBounds. Each rectangle is [x, y, width, height] where x/width are time position/duration and y/height are note number (127 at top, 1/128 height per note). Returns empty array if no sequence is loaded. Useful for drawing a piano roll in a ScriptPanel paint routine.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var rects = mp.getNoteRectangleList([0, 0, 500, 200]);
```
Pair with: MidiPlayer.convertEventListToNoteRectangles -- same output but from arbitrary event list. MidiPlayer.connectToPanel -- auto-repaint panel on changes.
Source:
  ScriptingApiObjects.cpp:6250  getNoteRectangleList() -> HiseMidiSequence::getRectangleList(targetBounds)
