Sets the currently selected modulation source in exclusive source selection mode. This is typically handled automatically by the ModulationMatrixController FloatingTile, but can be called directly when building a custom UI replacement. Calling this also closes all currently active hover popups.

> [!Warning:Requires selectable sources to be enabled] Throws a script error if selectable sources mode is not active. Enable it either by calling `setMatrixModulationProperties({"SelectableSources": true})` or by registering a source selection callback.
