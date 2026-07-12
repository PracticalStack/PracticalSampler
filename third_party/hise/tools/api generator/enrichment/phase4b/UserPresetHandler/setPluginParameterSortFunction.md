UserPresetHandler::setPluginParameterSortFunction(Function customSortFunction) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Installs a custom sort function for DAW-visible plugin parameter ordering. The
callback receives two parameter objects and must return negative (first before
second), zero (equal), or positive (second before first). Pass a non-function
to reset to default sorting.
Callback signature: f(Object a, Object b)
  Each object has: type (int), parameterIndex (int), typeIndex (int),
  name (String), group (String)
Dispatch/mechanics:
  Stored on PluginParameterAudioProcessor::pluginParameterSortFunction
  Called synchronously via callSync when host queries parameter list
  Returning undefined/void falls back to default sort for that pair
Source:
  ScriptExpansion.cpp  setPluginParameterSortFunction()
    -> stores WeakCallbackHolder on PluginParameterAudioProcessor
  PluginParameterProcessor.cpp  sort comparator
    -> callSync with two DynamicObject args
