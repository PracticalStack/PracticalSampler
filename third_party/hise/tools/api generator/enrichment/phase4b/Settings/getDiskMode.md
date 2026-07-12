Settings::getDiskMode() -> Integer

Thread safety: SAFE
Returns the current disk streaming mode. 0 = SSD (larger preload buffer,
fast storage), 1 = HDD (stream-optimized, slower storage).

Pair with:
  setDiskMode -- change the disk mode

Source:
  ScriptingApi.cpp  Settings::getDiskMode()
    -> driver->diskMode (direct member read)
