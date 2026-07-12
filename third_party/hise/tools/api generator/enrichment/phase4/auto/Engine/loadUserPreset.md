Loads a user preset by relative path or ScriptFile object. The `.preset` extension is appended automatically. Relative paths are resolved against the UserPresets directory.

> [!Warning:Cannot call during onInit] Cannot be called during `onInit` - a script error is thrown. Load presets from runtime callbacks (button handlers, timer callbacks) instead.