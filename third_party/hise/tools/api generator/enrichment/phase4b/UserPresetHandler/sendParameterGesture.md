UserPresetHandler::sendParameterGesture(Integer automationType, Integer indexWithinType, Integer gestureActive) -> Integer

Thread safety: WARNING -- calls JUCE AudioProcessorParameter::beginChangeGesture/endChangeGesture; involves host-layer listener dispatch
Sends a parameter gesture begin/end message to the DAW host. Enables the host to
record automation correctly (most DAWs only record between gesture pairs).
Returns true if a matching parameter was found.
  automationType: 0=Macro, 1=CustomAutomation, 2=ScriptControl, 3=NKSWrapper
Dispatch/mechanics:
  Iterates registered plugin parameters -> finds match by type and index
    -> calls beginChangeGesture() or endChangeGesture() on the JUCE parameter
Pair with:
  setAutomationValue -- wrap value changes in gesture begin/end pairs
  setParameterGestureCallback -- observe gesture events from the host side
Source:
  ScriptExpansion.cpp  sendParameterGesture()
    -> iterates AudioProcessor::getParameters()
    -> HisePluginParameterBase type/index match
    -> beginChangeGesture() / endChangeGesture()
