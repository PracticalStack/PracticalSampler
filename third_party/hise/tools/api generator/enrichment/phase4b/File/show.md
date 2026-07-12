File::show() -> undefined

Thread safety: UNSAFE -- dispatches to message thread via MessageManager::callAsync. Performs OS file-reveal I/O.
Opens a native file explorer window (Explorer on Windows, Finder on macOS)
with this file or directory selected. Returns immediately without blocking.

Dispatch/mechanics:
  auto f_ = f;
  MessageManager::callAsync([f_]() { f_.revealToUser(); });

Source:
  ScriptingApiObjects.cpp  ScriptFile::show()
    -> MessageManager::callAsync -> f.revealToUser()
