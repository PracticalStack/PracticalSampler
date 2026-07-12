ScriptImage::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the current attribute value from the connected processor (via processorId
and parameterId properties) and calls setValue() with that value. Does nothing
if no processor connection is established.
Special parameterId values: -2 = modulation intensity, -3 = bypass state,
-4 = inverted bypass, >= 0 = attribute at index.
Pair with:
  setValue -- called internally with the read value
Source:
  ScriptingApiContent.cpp  ScriptComponent::updateValueFromProcessorConnection()
    -> reads processor attribute/bypass/modulation -> setValue()
