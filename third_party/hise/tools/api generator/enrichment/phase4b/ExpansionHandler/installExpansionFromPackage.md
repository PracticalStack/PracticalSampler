ExpansionHandler::installExpansionFromPackage(var packageFile, var sampleDirectory) -> bool

Thread safety: UNSAFE -- kills voices via killVoicesAndCall, runs on SampleLoadingThread,
performs file I/O and HLAC decompression.
Installs an expansion from an .hr archive. Decompresses into the expansion directory,
optionally encrypts if credentials are set, then reinitialises all expansions.
sampleDirectory accepts FileSystem.Expansions, FileSystem.Samples, or a File object.
Required setup:
  const var eh = Engine.createExpansionHandler();
  eh.setInstallCallback(onInstallProgress);
Dispatch/mechanics:
  killVoicesAndCall -> SampleLoadingThread:
    -> creates expansion directory structure under {ProjectRoot}/Expansions/
    -> HlacArchiver decompresses .hr archive into sample directory
    -> if credentials set: encrypts to .hxp; otherwise renames to .hxi
    -> forceReinitialisation() -> expansionInstalled() notification
Pair with:
  setInstallCallback -- track installation progress
  setCredentials -- auto-encrypts intermediate file during install
  getExpansionForInstallPackage -- check if already installed before installing
Anti-patterns:
  - Do NOT pass an invalid sampleDirectory type -- if neither a FileSystem constant
    nor a File object, reports "The sample directory does not exist"
Source:
  ScriptExpansion.cpp:1323  installExpansionFromPackage()
    -> ExpansionHandler::installFromResourceFile() (ExpansionHandler.cpp:409)
    -> killVoicesAndCall -> SampleLoadingThread execution
