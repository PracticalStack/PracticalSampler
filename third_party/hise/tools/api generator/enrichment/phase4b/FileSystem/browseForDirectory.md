FileSystem::browseForDirectory(var startFolder, Function callback) -> undefined

Thread safety: UNSAFE -- opens native directory dialog via MessageManager::callAsync, heap allocation, UI thread dispatch
Opens a native directory browser dialog for selecting a single directory. Runs asynchronously;
delivers the selected File object (pointing to the directory) to the callback. Callback is
not invoked if user cancels. Subject to the same static re-entry guard as all browse methods.
Callback signature: f(File directory)

Dispatch/mechanics:
  Resolves startFolder (int constant or ScriptFile only, NOT path strings)
  -> browseInternally(file, false, true, "", callback, false)
  -> MessageManager::callAsync -> FileChooser.browseForDirectory
  -> wraps result in ScriptFile, calls WeakCallbackHolder

Pair with:
  browse -- for file selection instead of directory
  browseForMultipleDirectories -- for multi-directory selection
  fromAbsolutePath -- convert path string to File for use as startFolder

Anti-patterns:
  - Do NOT pass a path string as startFolder -- only accepts SpecialLocations constants or
    File objects. Strings silently produce an empty start directory. Use fromAbsolutePath() first.

Source:
  ScriptingApi.cpp:7394  FileSystem::browseForDirectory()
    -> browseInternally() with static fileChooserIsOpen guard
    -> MessageManager::callAsync -> FileChooser.browseForDirectory -> WeakCallbackHolder
