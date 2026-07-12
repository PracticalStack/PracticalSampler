Expansion::getUserPresetList() -> Array

Thread safety: UNSAFE -- performs recursive filesystem scan for .preset files
Returns user preset names for this expansion. Scans the UserPresets subdirectory
recursively for .preset files. Returns relative paths with .preset extension stripped
and backslashes normalized to forward slashes.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  exp->getSubDirectory(UserPresets).findChildFiles(File::findFiles, true, "*.preset")
    -> converts absolute paths to relative paths from UserPresets folder
    -> strips .preset extension, normalizes backslashes to forward slashes

Pair with:
  rebuildUserPresets -- extract presets from encoded expansion data before listing

Source:
  ScriptExpansion.cpp:1542  ScriptExpansionReference wrapper
    -> filesystem scan (NOT pool-based, unlike other list methods)
    -> getSubDirectory(FileHandlerBase::UserPresets).findChildFiles()
