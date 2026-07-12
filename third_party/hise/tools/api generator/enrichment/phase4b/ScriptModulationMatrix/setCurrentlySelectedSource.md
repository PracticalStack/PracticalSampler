ScriptModulationMatrix::setCurrentlySelectedSource(String sourceId) -> undefined

Thread safety: UNSAFE -- calls setExlusiveMatrixSource which modifies container state.
Sets the currently selected modulation source in exclusive source selection mode.
Fires the source selection callback if one is registered. Throws a script error
if selectable sources are not enabled.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");
  mm.setMatrixModulationProperties({"SelectableSources": true});

Pair with:
  setSourceSelectionCallback -- observe selection changes
  setMatrixModulationProperties -- enable SelectableSources mode first

Anti-patterns:
  - Do NOT call without enabling selectable sources first -- throws a script error.
    Enable via setMatrixModulationProperties({"SelectableSources": true}) or by
    registering a setSourceSelectionCallback (which enables it as a side effect).

Source:
  ScriptModulationMatrix.cpp  setCurrentlySelectedSource()
    -> checks matrixProperties.selectableSources
    -> container->setExlusiveMatrixSource(sourceIndex)
