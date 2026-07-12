Returns `true` when the current note-on occurs while another key is already held, indicating a legato transition. In `onNoteOn`, the key count is already updated before the callback fires, so the first solo note returns `false` and overlapping notes return `true`.

> [!Warning:Also true when zero keys pressed] Also returns `true` when zero keys are pressed (e.g. in `onNoteOff` after the last key is released). The internal check is `numPressedKeys != 1`, not `numPressedKeys > 1`. Always combine with a `getNumPressedKeys() > 0` or `lastNote != -1` check when using this in note-off callbacks.
