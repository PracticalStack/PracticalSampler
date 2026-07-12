Restores all component values from a saved preset file. The path can be absolute or relative to the UserPresets folder. Components with `saveInPreset` set to false are skipped.

> [!Warning:Filename must match embedded identifier] In exported plugins, the filename is matched against an embedded identifier. A mismatch throws a "Preset ID not found" error, so ensure the filename matches exactly.