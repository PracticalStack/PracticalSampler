Unlocker::isUnlocked() -> Integer

Thread safety: SAFE
Returns whether the plugin is currently unlocked (licensed). Delegates to
juce::OnlineUnlockStatus::isUnlocked() which checks if a valid RSA-signed key
file has been loaded. Also used internally by CHECK_COPY_AND_RETURN_N macros
to gate audio processing in unlicensed builds.

Pair with:
  loadKeyFile -- validates the key file and sets unlock status
  writeKeyFile -- writes key data to disk (call loadKeyFile after to unlock)

Source:
  ScriptExpansion.cpp  RefObject::isUnlocked()
    -> juce::OnlineUnlockStatus::isUnlocked()
