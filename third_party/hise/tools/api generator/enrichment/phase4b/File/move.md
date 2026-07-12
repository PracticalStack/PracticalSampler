File::move(ScriptObject target) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (file move/rename operation).
Moves this file to the location specified by the target File object.
Returns true on success, false otherwise.

Dispatch/mechanics:
  dynamic_cast<ScriptFile*>(target) -> f.moveFileTo(sf->f)
  Reports script error "target is not a file" if target is not a File object.

Pair with:
  copy -- to duplicate instead of relocate
  rename -- for same-directory renaming

Anti-patterns:
  - After moving, the original File object still references the old path. The
    internal juce::File is immutable. Use the target File for the new location.
  - The target must be a File object, not a string path.

Source:
  ScriptingApiObjects.cpp  ScriptFile::move()
    -> dynamic_cast<ScriptFile*>(target.getObject())
    -> f.moveFileTo(sf->f)
