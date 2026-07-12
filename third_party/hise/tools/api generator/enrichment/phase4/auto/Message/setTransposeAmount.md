Sets the transpose amount in semitones on the current event. The transpose shifts which sample zone is triggered without changing the raw note number returned by `Message.getNoteNumber()`. The transposed pitch is `getNoteNumber() + getTransposeAmount()`. Transpose is automatically copied from note-on to note-off, so you only need to set it in `onNoteOn`.

For timbre shifting (changing which sample plays without changing the audible pitch), pair this with `Message.setCoarseDetune()` using the opposite sign to cancel the pitch change.

> [!Warning:Pair with setCoarseDetune for timbre shift] Always pair `setTransposeAmount(-N)` with `setCoarseDetune(N)` for timbre shifting. If coarse detune is omitted, the note sounds at the transposed pitch rather than the original.
