ScriptComboBox::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the current attribute value from the connected processor (set via
processorId and parameterId properties) and calls setValue() with that value.
Does nothing if no processor connection is established.
Dispatch/mechanics:
  Reads parameterId: -2 = modulation intensity, -3 = bypass state (1.0/0.0),
  -4 = inverted bypass, >= 0 = attribute at parameter index
  -> calls setValue() with the read value
Pair with:
  setValue -- the underlying value setter called internally
Source:
  ScriptingApiContent.h  ScriptComponent::updateValueFromProcessorConnection()
