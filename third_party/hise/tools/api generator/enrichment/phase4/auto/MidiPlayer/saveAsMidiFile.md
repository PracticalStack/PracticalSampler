Saves the current sequence to a MIDI file at the given path, writing data to the specified track index. The MIDI file pool is reloaded automatically after saving, so the file is immediately available via `getMidiFileList()`. Returns true on success.

> [!Warning:Overwrites target file permanently] This operation is not undoable and permanently overwrites the target file. The original content cannot be recovered unless you have a separate backup.
