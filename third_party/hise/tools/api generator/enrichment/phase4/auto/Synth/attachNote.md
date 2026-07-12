Links an artificial note to an original note so that when the original receives a note-off, the artificial note is automatically stopped as well. This eliminates the need for manual note-off tracking in `onNoteOff` when layering artificial notes on top of incoming events. Returns `true` if the attachment succeeded.

> [!Warning:Requires setFixNoteOnAfterNoteOff first] Requires `Synth.setFixNoteOnAfterNoteOff(true)` to be called first, typically in `onInit`. Without it, a script error is thrown.
