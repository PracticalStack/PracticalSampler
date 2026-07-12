Registers a callback that fires when the currently selected modulation source changes, either from clicking a dragger in the ModulationMatrixController or from calling `setCurrentlySelectedSource()`. The callback receives a single argument: the source ID string.

> [!Warning:Implicitly toggles selectable sources mode] Registering a callback automatically enables selectable sources mode. Passing a non-function value to clear the callback automatically disables it. This implicit state change is not obvious from the method name.
