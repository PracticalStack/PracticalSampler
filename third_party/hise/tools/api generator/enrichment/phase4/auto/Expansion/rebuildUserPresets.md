Extracts user presets from the encoded expansion data and writes them to disk. Only works with Intermediate or Encrypted expansion types - FileBased expansions have no encoded data to extract from.

If the expansion contains user presets, they are extracted automatically during the first installation. However, updating an existing expansion does not overwrite previously extracted presets. Call this method after an expansion update to deploy new or modified presets. The most convenient place for this call is inside the callback registered with `ExpansionHandler.setInstallCallback()`.

> [!Warning:Silently fails on FileBased expansions] Returns `false` with no error for FileBased expansions. Check the type with `getExpansionType()` before calling.

> [!Warning:Overwrites existing preset files] Forces extraction even if presets already exist on disk. There is no way to merge or preserve user modifications to previously extracted presets.
