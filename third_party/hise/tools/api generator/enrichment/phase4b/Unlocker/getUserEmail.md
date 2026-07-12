Unlocker::getUserEmail() -> String

Thread safety: WARNING -- String involvement, atomic ref-count operations on the returned string.
Returns the email address from the loaded license key. Delegates to JUCE
OnlineUnlockStatus email extraction from RSA-validated key data. Returns empty
string if no valid key file has been loaded.

Pair with:
  loadKeyFile -- must be called first to populate email
  getRegisteredMachineId -- companion identity field from the same key file

Source:
  ScriptExpansion.cpp  RefObject::getUserEmail()
    -> delegates to juce::OnlineUnlockStatus email extraction
