ScriptedViewport::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the current attribute value from the connected processor (via processorId and parameterId properties) and calls setValue() with that value. Does nothing if no processor connection is established.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.updateValueFromProcessorConnection();
Pair with: setValue (called internally with the processor's current value)
Source:
  ScriptingApiContent.cpp  ScriptComponent::updateValueFromProcessorConnection() -> getConnectedProcessor()->getAttribute() -> setValue()
