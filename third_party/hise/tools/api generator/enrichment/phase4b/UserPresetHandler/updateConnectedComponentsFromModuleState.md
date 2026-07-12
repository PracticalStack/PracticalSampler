UserPresetHandler::updateConnectedComponentsFromModuleState() -> undefined

Thread safety: WARNING -- iterates all UI components, reads processor attributes, calls setValue with value change notifications
Refreshes values of all UI components connected to processor parameters via
processorId/parameterId properties. Reads current parameter values from
processors and pushes them to UI components. Useful after programmatically
changing module parameters to sync the UI.
Dispatch/mechanics:
  Iterates all components in scripting content
    -> for each with valid processor connection: updateValueFromProcessorConnection()
    -> connectedParameterIndex >= 0: getAttribute(index)
    -> connectedParameterIndex == -2: getIntensity() (modulation)
    -> connectedParameterIndex == -3: isBypassed() inverted
    -> connectedParameterIndex == -4: isBypassed() normal
    -> calls setValue() on the component
Pair with:
  updateSaveInPresetComponents -- restore from saved component state object
  createObjectForSaveInPresetComponents -- capture component state
Source:
  ScriptExpansion.cpp  updateConnectedComponentsFromModuleState()
    -> iterates content->getNumComponents()
    -> ScriptComponent::updateValueFromProcessorConnection()
