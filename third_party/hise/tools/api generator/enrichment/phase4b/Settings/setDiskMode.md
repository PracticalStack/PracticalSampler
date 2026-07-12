Settings::setDiskMode(Integer mode) -> undefined

Thread safety: UNSAFE -- sets the disk mode on the sample manager, may trigger streaming config changes
Sets the disk streaming mode. 0 = SSD (larger preload buffers, fast storage),
1 = HDD (stream-optimized, slower storage).

Anti-patterns:
  - [BUG] No range validation -- values other than 0 or 1 are accepted silently and
    cast directly to the internal DiskMode enum, producing undefined behavior

Pair with:
  getDiskMode -- read the current mode

Source:
  ScriptingApi.cpp  Settings::setDiskMode()
    -> driver->diskMode = mode
    -> mc->getSampleManager().setDiskMode(...)
