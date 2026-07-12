FileSystem::browseForMultipleFiles(var startFolder, String wildcard, Function callback) -> undefined

Thread safety: UNSAFE -- opens native file dialog via MessageManager::callAsync, heap allocation, UI thread dispatch
Opens a native file browser dialog that allows selecting multiple files at once. Callback
receives an Array of File objects. Callback is not invoked if user cancels. Subject to the
same static re-entry guard as all browse methods. Always opens a file-open dialog (never save).
Callback signature: f(Array files)

Dispatch/mechanics:
  Resolves startFolder via getFileFromVar (accepts int constant, ScriptFile, or path string)
  -> browseInternally(file, false, false, wildcard, callback, true)
  -> MessageManager::callAsync -> FileChooser.browseForMultipleFilesToOpen
  -> wraps results in Array of ScriptFile objects, calls WeakCallbackHolder

Pair with:
  browse -- for single-file selection
  browseForMultipleDirectories -- for multi-directory selection

Anti-patterns:
  - Do NOT treat the callback argument as a single File -- it is always an Array, even
    when only one file is selected. Always iterate the result.

Source:
  ScriptingApi.cpp:7394  FileSystem::browseForMultipleFiles()
    -> getFileFromVar(startFolder) -- accepts path strings unlike browse/browseForDirectory
    -> browseInternally() with multiple=true -> FileChooser.browseForMultipleFilesToOpen
