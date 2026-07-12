Unlocker::unlockExpansionList(var expansionIdList) -> Integer

Thread safety: UNSAFE -- File I/O scanning expansion folders, XML parsing, string operations.
Backend-only method that scans expansion folders for metadata, collects encryption
keys, and passes them to ExpansionHandler.setCredentials(). Silently returns false
in frontend builds without performing any work.

Dispatch/mechanics:
  Iterates expansion folders -> reads project_info.xml (FullInstrumentExpansion)
    or expansion_info.xml (standard) -> extracts encryption keys
    -> ExpansionHandler.setCredentials(collectedKeys)

Pair with:
  loadExpansionList -- frontend equivalent that uses encrypted expansion list file
  writeExpansionKeyFile -- write server-provided expansion data for frontend use

Anti-patterns:
  - Silently returns false in frontend builds with no error or warning. Code that
    relies on this method works in the HISE IDE but silently fails in exported plugins.

Source:
  ScriptExpansion.cpp  RefObject::unlockExpansionList()
    -> USE_BACKEND only: scans expansion folders for metadata
    -> ExpansionHandler.setCredentials()
