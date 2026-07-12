Unlocker::getRegisteredMachineId() -> String

Thread safety: WARNING -- String involvement, atomic ref-count operations on the returned string.
Returns the machine ID extracted from the loaded license key file. Parsed from the
"Machine numbers: " line during loadKeyFile(). Returns empty string if no key file
has been loaded or if the key file lacks a machine numbers line.

Pair with:
  loadKeyFile -- must be called first to populate machine ID
  getUserEmail -- companion identity field from the same key file

Source:
  ScriptExpansion.cpp  RefObject::getRegisteredMachineId()
    -> returns registeredMachineId string parsed during loadKeyFile()
