Engine::setUserPresetTagList(Array listOfTags) -> undefined

Thread safety: UNSAFE -- StringArray construction, heap operations
Sets the tag categories for the user preset browser filtering.
Anti-patterns:
  - Non-array argument silently does nothing with no error
Source:
  ScriptingApi.cpp  Engine::setUserPresetTagList()
    -> UserPresetHandler::setTagList(tags)
