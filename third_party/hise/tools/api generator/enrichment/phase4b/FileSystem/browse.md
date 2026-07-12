FileSystem::browse(var startFolder, Integer forSaving, String wildcard, Function callback) -> undefined

Thread safety: UNSAFE -- opens native file dialog via MessageManager::callAsync, heap allocation, UI thread dispatch
Opens a native file browser dialog for selecting a single file. Runs asynchronously;
delivers the selected File object to the callback. Callback is not invoked if user cancels.
Only one file dialog can be open at a time -- subsequent calls are silently dropped.
Callback signature: f(File file)

Dispatch/mechanics:
  Resolves startFolder (int constant or ScriptFile only, NOT path strings)
  -> browseInternally(file, forSaving, false, wildcard, callback, false)
  -> MessageManager::callAsync -> FileChooser.browseForFileToOpen/Save
  -> wraps result in ScriptFile, calls WeakCallbackHolder

Pair with:
  browseForDirectory -- for directory selection instead of file
  browseForMultipleFiles -- for multi-file selection
  fromAbsolutePath -- convert path string to File for use as startFolder

Anti-patterns:
  - Do NOT pass a path string as startFolder -- unlike browseForMultipleFiles, this method
    only accepts SpecialLocations constants or File objects. Strings silently produce an
    empty start directory.
  - Do NOT call browse while another file dialog is open -- silently ignored due to static
    re-entry guard. No error or callback invocation.

Source:
  ScriptingApi.cpp:7394  FileSystem::browse()
    -> browseInternally() with static fileChooserIsOpen guard
    -> MessageManager::callAsync -> FileChooser -> WeakCallbackHolder
