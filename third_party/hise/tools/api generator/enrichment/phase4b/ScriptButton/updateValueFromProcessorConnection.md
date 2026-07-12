ScriptButton::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the current attribute value from the connected processor (set via processorId
and parameterId properties) and calls setValue() with that value. Does nothing if
no processor connection is established.

Special parameterId values: -2 = modulation intensity, -3 = bypass state (1.0 if
bypassed), -4 = inverted bypass state (0.0 if bypassed), >= 0 = attribute index.

Pair with:
  setValue -- called internally with the read value

Source:
  ScriptingApiContent.cpp  ScriptComponent::updateValueFromProcessorConnection()
    -> reads processor attribute/bypass -> setValue()
