ExpansionHandler::setInstallFullDynamics(var shouldInstallFullDynamics) -> undefined

Thread safety: SAFE -- simple boolean assignment on the core ExpansionHandler.
Controls whether full dynamic range samples are extracted during expansion
installation. When true, extracts uncompressed sample data instead of HLAC-compressed.
Must be configured before calling installExpansionFromPackage().
Pair with:
  installExpansionFromPackage -- respects this setting during extraction
Source:
  ScriptExpansion.cpp  setInstallFullDynamics()
    -> ExpansionHandler::setInstallFullDynamics(bool)
