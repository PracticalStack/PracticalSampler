File::setExecutePermission(Integer shouldBeExecutable) -> Integer

Thread safety: UNSAFE -- modifies filesystem permissions (OS I/O call).
Sets or clears the execute permission on this file. Returns true on success.
On Windows, this has no practical effect -- executability is determined by file
extension. Primarily useful on macOS and Linux.

Source:
  ScriptingApiObjects.cpp  ScriptFile::setExecutePermission()
    -> f.setExecutePermission(shouldBeExecutable)
