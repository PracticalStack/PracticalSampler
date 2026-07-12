ExpansionHandler::getExpansionForInstallPackage(var packageFile) -> ScriptObject

Thread safety: UNSAFE -- File I/O to read archive metadata, heap allocation for wrapper.
Checks whether an expansion from the given .hr package is already installed. Returns
the expansion reference if found, undefined otherwise. FileBased expansions are
deliberately excluded from detection.
Dispatch/mechanics:
  Reads package metadata to determine target folder name
    -> searches expansionList for matching folder
    -> excludes FileBased type (simulates end-user install flow)
Pair with:
  installExpansionFromPackage -- install if not already present
  getMetaDataFromPackage -- read package info without installing
Anti-patterns:
  - Do NOT expect FileBased expansions to be detected -- they are deliberately
    excluded to simulate end-user flow where only encoded/encrypted types count
Source:
  ScriptExpansion.cpp:1371  getExpansionForInstallPackage()
    -> reads HlacArchiver metadata -> checks expansion folder existence
