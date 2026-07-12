Registers this table's owned data in a parent processor slot and returns a handle you can reuse outside the UI layer. This is the standard bridge when playback callbacks need the same curve shown in the editor.

> [!Warning:Register once and cache the handle] Register once during setup and cache the handle. Re-registering in note or control callbacks adds avoidable runtime overhead.
