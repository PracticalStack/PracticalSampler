ScriptModulationMatrix::setSourceSelectionCallback(Function newCallback) -> undefined

Thread safety: UNSAFE -- constructs a WeakCallbackHolder, modifies container state, registers broadcaster listener.
Registers a callback that fires whenever a new modulation source is selected in
exclusive source mode. Registering a callback automatically enables selectable
sources mode. Passing a non-function value disables selectable sources mode.
Callback signature: f(String sourceName)

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  WeakCallbackHolder(1 arg) -> connected to container->currentMatrixSourceBroadcaster
    -> side effect: sets matrixProperties.selectableSources = true
    -> passing non-function: sets selectableSources = false, removes listener

Pair with:
  setCurrentlySelectedSource -- programmatically change the selected source
  setMatrixModulationProperties -- alternative way to enable SelectableSources

Anti-patterns:
  - Registering this callback implicitly enables selectable sources mode. Clearing
    it implicitly disables it. This side effect is not obvious from the method name.

Source:
  ScriptModulationMatrix.cpp  setSourceSelectionCallback()
    -> WeakCallbackHolder with 1 arg
    -> container->currentMatrixSourceBroadcaster (LambdaBroadcaster<int>)
    -> toggles container->matrixProperties.selectableSources
