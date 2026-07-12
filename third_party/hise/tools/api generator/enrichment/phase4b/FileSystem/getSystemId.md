FileSystem::getSystemId() -> String

Thread safety: WARNING -- string construction with atomic ref-count operations
Returns a unique machine identifier string for the current computer. Deterministic --
same value on repeated calls. Derived from hardware characteristics via JUCE's
OnlineUnlockStatus::MachineIDUtilities. Returns the first entry from the local machine
ID list as a hex string.

Pair with:
  encryptWithRSA / decryptWithRSA -- machine ID can be encrypted for secure transmission

Source:
  ScriptingApi.cpp:7394  FileSystem::getSystemId()
    -> OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()[0]
