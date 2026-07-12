FileSystem::browseForMultipleDirectories(var startFolder, Function callback) -> undefined

Thread safety: UNSAFE -- opens native directory dialog via MessageManager::callAsync, heap allocation, UI thread dispatch
Opens a native directory browser dialog that allows selecting multiple directories at once.
Callback receives an Array of File objects. Callback is not invoked if user cancels.
Subject to the same static re-entry guard as all browse methods.
Callback signature: f(Array directories)

Dispatch/mechanics:
  Resolves startFolder via getFileFromVar (accepts int constant, ScriptFile, or path string)
  -> browseInternally(file, false, true, "", callback, true)
  -> MessageManager::callAsync -> FileChooser.browseForMultipleDirectories
  -> wraps results in Array of ScriptFile objects, calls WeakCallbackHolder

Pair with:
  browseForDirectory -- for single-directory selection
  browseForMultipleFiles -- for multi-file selection

Anti-patterns:
  - Do NOT treat the callback argument as a single File -- it is always an Array, even
    when only one directory is selected. Always iterate the result.

Source:
  ScriptingApi.cpp:7394  FileSystem::browseForMultipleDirectories()
    -> getFileFromVar(startFolder) -- accepts path strings unlike browse/browseForDirectory
    -> browseInternally() with multiple=true -> FileChooser.browseForMultipleDirectories
