ExpansionHandler (object)
Obtain via: Engine.createExpansionHandler()

Factory and manager for loading, installing, encrypting, and switching expansion
packs at runtime. Manages expansion discovery in the Expansions folder, credential-
based encryption for .hxp files, installation from .hr archives with progress
tracking, and active expansion switching with callbacks.

Constants:
  ExpansionType:
    FileBased = 0        Plain folder expansion with expansion_info.xml
    Intermediate = 1     Encoded .hxi expansion format
    Encrypted = 2        Credential-encrypted .hxp expansion format

Complexity tiers:
  1. Browsing and switching: getExpansionList, setCurrentExpansion,
     setExpansionCallback, getCurrentExpansion. Build a selector, react to
     expansion changes.
  2. Resource enumeration: + Expansion.getSampleMapList, getAudioFileList,
     getImageList, getWildcardReference. Browse and load expansion-specific content.
  3. Installation pipeline: + installExpansionFromPackage, setInstallCallback,
     getMetaDataFromPackage, getExpansionForInstallPackage. In-app install with
     progress tracking.
  4. Encrypted distribution: + setCredentials, encodeWithCredentials,
     setAllowedExpansionTypes. Credential-encrypted delivery for production builds.

Practical defaults:
  - Always set setExpansionCallback() before the first setCurrentExpansion() so
    the callback fires on the initial switch.
  - Call the expansion callback logic manually with undefined at init time to set
    the default no-expansion UI state.
  - Pass empty string to setCurrentExpansion("") to deactivate and restore base
    product state.
  - Use setAllowedExpansionTypes([eh.Intermediate, eh.Encrypted]) in production
    to hide FileBased development folders from end users.

Common mistakes:
  - Setting up the expansion callback after setCurrentExpansion() -- the initial
    switch won't fire the callback.
  - Not checking isDefined(e) in the expansion callback -- receives undefined
    when expansion is cleared via setCurrentExpansion("").
  - Using setEncryptionKey() -- hard-deprecated, always throws. Configure the
    BlowFish key in Project Settings instead.
  - Passing a string to setCredentials() -- requires a JSON object, not a string.
    Non-object triggers error via error function, not a script error.
  - Passing a single value to setAllowedExpansionTypes() -- requires an array
    of type constants, not a single integer.

Example:
  const var eh = Engine.createExpansionHandler();

  eh.setExpansionCallback(function(e)
  {
      if (isDefined(e))
          Console.print("Loaded expansion: " + e.getProperties().Name);
      else
          Console.print("No expansion active");
  });

  eh.setErrorFunction(function(message, isCritical)
  {
      Console.print((isCritical ? "ERROR: " : "Warning: ") + message);
  });

  const var list = eh.getExpansionList();

Methods (16):
  encodeWithCredentials          getCurrentExpansion
  getExpansion                   getExpansionForInstallPackage
  getExpansionList               getMetaDataFromPackage
  installExpansionFromPackage    refreshExpansions
  setAllowedExpansionTypes       setCredentials
  setCurrentExpansion            setErrorFunction
  setErrorMessage                setExpansionCallback
  setInstallCallback             setInstallFullDynamics
