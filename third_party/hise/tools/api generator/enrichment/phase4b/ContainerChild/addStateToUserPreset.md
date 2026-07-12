ContainerChild::addStateToUserPreset(Integer shouldAdd) -> undefined

Thread safety: UNSAFE
Registers or unregisters this ContainerChild with the UserPresetHandler as a
state manager. When registered, the component's entire subtree is automatically
saved/restored with user presets via Base64 serialization. Pass false to unregister.
Pair with:
  toBase64 -- manual serialization using the same mechanism
  fromBase64 -- manual restoration using the same mechanism
Source:
  ScriptingApiContent.cpp  ChildReference::addStateToUserPreset()
    -> UserPresetHandler::addStateManager() / removeStateManager()
    -> exportAsValueTree() uses toBase64(true)
    -> restoreFromValueTree() uses fromBase64()
