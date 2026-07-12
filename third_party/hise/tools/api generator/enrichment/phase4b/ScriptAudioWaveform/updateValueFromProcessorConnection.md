ScriptAudioWaveform::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Reads the current attribute from the connected processor (via processorId and
parameterId properties) and calls setValue() with that value. Does nothing if
no processor connection is established.

Special parameterId values: -2 = modulation intensity, -3 = bypass state,
-4 = inverted bypass state, >= 0 = attribute at that index.

Source:
  ScriptingApiContent.cpp  ScriptComponent::updateValueFromProcessorConnection()
    -> getConnectedProcessor()->getAttribute(parameterId) -> setValue()
