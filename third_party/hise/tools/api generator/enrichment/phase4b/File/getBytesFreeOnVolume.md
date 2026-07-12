File::getBytesFreeOnVolume() -> Integer

Thread safety: UNSAFE -- queries filesystem volume information (OS I/O call).
Returns the number of bytes of free disk space on the volume containing this file.
Returns 0 if the volume cannot be determined.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getBytesFreeOnVolume()
    -> f.getBytesFreeOnVolume()
