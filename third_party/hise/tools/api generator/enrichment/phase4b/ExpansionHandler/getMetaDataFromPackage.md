ExpansionHandler::getMetaDataFromPackage(var packageFile) -> JSON

Thread safety: UNSAFE -- File I/O to read HLAC archive metadata.
Reads metadata from an .hr archive package without installing it. Returns a JSON
object with package info including the HxiName property identifying the target
expansion.
Anti-patterns:
  - [BUG] Passing a non-File argument silently returns undefined instead of
    reporting an error (unlike similar methods that throw script errors)
Pair with:
  getExpansionForInstallPackage -- check if already installed
  installExpansionFromPackage -- install after inspecting metadata
Source:
  ScriptExpansion.cpp:1360  getMetaDataFromPackage()
    -> HlacArchiver::readMetadataFromArchive()
