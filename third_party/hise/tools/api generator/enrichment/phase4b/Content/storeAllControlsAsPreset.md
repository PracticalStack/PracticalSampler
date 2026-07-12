Content::storeAllControlsAsPreset(String fileName, var automationData) -> void

Thread safety: UNSAFE
Saves all component control values to an XML data file. Counterpart to
restoreAllControlsFromPreset(). The fileName is resolved relative to the UserPresets
folder. Pass undefined for automationData to store all controls.

Pair with:
  restoreAllControlsFromPreset -- load back the saved values

Anti-patterns:
  - The file is written relative to the UserPresets directory. Using absolute paths or
    paths outside this directory may not work as expected.

Source:
  ScriptingApiContent.cpp  Content::storeAllControlsAsPreset()
    -> exports contentPropertyData as ValueTree
    -> merges with existing file if present
