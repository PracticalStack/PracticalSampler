File::getRedirectedFolder() -> ScriptObject

Thread safety: UNSAFE -- reads a link file from disk to resolve the redirect target (I/O).
Checks this directory for a HISE link file and returns the redirect target if found.
HISE uses platform-specific link files (LinkWindows, LinkOSX, LinkLinux) containing
the absolute path to a redirect target. Returns this same File if no redirect exists.

Dispatch/mechanics:
  if f.existsAsFile() -> reportScriptError (must be a directory)
  if !f.isDirectory() -> return this (path does not exist, no error)
  FileHandlerBase::getFolderOrRedirect(f) -> checks for LinkWindows/LinkOSX/LinkLinux
    -> reads link file as plain text -> returns redirect target or original

Anti-patterns:
  - Calling on a file path that exists as a regular file reports a script error.
    However, if the path does not exist at all, returns this silently (no error).
  - Link file names are platform-specific. LinkWindows on Windows, LinkOSX on macOS,
    LinkLinux on Linux. Cross-platform link files are ignored.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getRedirectedFolder()
    -> FileHandlerBase::getFolderOrRedirect(f)
    -> getLinkFile() returns platform-specific child file
    -> reads link file as string, returns File(target) if directory
