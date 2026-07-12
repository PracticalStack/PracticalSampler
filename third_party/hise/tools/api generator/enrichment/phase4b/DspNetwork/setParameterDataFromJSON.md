DspNetwork::setParameterDataFromJSON(JSON jsonData) -> Integer

Thread safety: UNSAFE -- performs string operations (toString, substring extraction), node/parameter lookups, and ValueTree property changes with undo manager.
Sets multiple node parameters from a JSON object. Each property key uses
nodeId.parameterId format (e.g. "myGain.Gain"). Values are cast to double.
Matched parameters are updated via ValueTree with undo support and marked as "probed".
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Pair with:
  setForwardControlsToParameters -- controls whether UI values also drive parameters
  undo -- to reverse parameter changes made by this method
Anti-patterns:
  - Do NOT rely on the return value to detect unmatched parameters -- [BUG] always
    returns true regardless of whether any parameters were found and set. Unmatched
    node IDs or parameter IDs are silently ignored.
Source:
  DspNetwork.cpp  setParameterDataFromJSON()
    -> iterates JSON properties, splits key at "." into nodeId + parameterId
    -> looks up node by nodeId, then parameter by parameterId
    -> sets ValueTree "Value" property with undo manager
    -> marks parameter as probed
    -> BUG: returns true unconditionally instead of computed ok flag
