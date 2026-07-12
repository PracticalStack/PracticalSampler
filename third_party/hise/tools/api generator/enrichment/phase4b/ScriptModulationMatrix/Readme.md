ScriptModulationMatrix (object)
Obtain via: Engine.createModulationMatrix(containerId)

Dynamic many-to-many modulation routing between global modulator sources and
UI/processor targets. Manages connection state as a ValueTree with user preset
integration, per-connection properties (intensity, mode, inversion, aux source),
and callbacks for connection changes, drag interaction, and source selection.

Complexity tiers:
  1. Minimal setup: Engine.createModulationMatrix, built-in FloatingTile panels
     (ModulationMatrix, ModulationMatrixController), automatic preset persistence.
     Only 1-2 lines of script beyond module tree setup.
  2. Configured defaults: + setMatrixModulationProperties. Define per-target
     default intensities, modulation modes, and range presets for drag-and-drop
     connection creation.
  3. Custom interaction layer: + setConnectionCallback, setDragCallback,
     setEditCallback, setSourceSelectionCallback, getModulationDisplayData.
     Fully custom modulation UIs with ScriptPanel rendering and context menus.

Practical defaults:
  - Use built-in FloatingTile panels (ModulationMatrixController,
    ModulationMatrix) as a starting point. Custom scripting only needed for
    non-standard interaction patterns.
  - Range presets like "FilterFreq" and "Gain0dB" handle the most common target
    types. Use custom range objects only when presets do not match.
  - When setting DefaultInitValues, always include both "Intensity" and "Mode"
    together. A non-zero intensity without a mode causes a script error.

Common mistakes:
  - Passing a non-GlobalModulatorContainer processor ID to createModulationMatrix
    -- causes a script error. Must be a GlobalModulatorContainer.
  - Calling setCurrentlySelectedSource without enabling SelectableSources first
    -- throws a script error. Enable via setMatrixModulationProperties or
    setSourceSelectionCallback.
  - Setting DefaultInitValues with non-zero Intensity but no Mode property --
    throws a script error because the system cannot determine modulation type.
  - Creating the matrix object but never placing FloatingTile panels or
    registering callbacks -- users have no way to create or manage connections.

Example:
  // Create a modulation matrix connected to the global modulator container
  const mm = Engine.createModulationMatrix("Global Modulator Container0");

  // Set up a connection callback
  mm.setConnectionCallback(function(sourceId, targetId, wasAdded)
  {
      Console.print(sourceId + " -> " + targetId + (wasAdded ? " added" : " removed"));
  });

Methods (19):
  canConnect                    clearAllConnections
  connect                       fromBase64
  getComponent                  getConnectionProperty
  getMatrixModulationProperties getModulationDisplayData
  getSourceList                 getTargetId
  getTargetList                 setConnectionCallback
  setConnectionProperty         setCurrentlySelectedSource
  setDragCallback               setEditCallback
  setMatrixModulationProperties setSourceSelectionCallback
  toBase64
