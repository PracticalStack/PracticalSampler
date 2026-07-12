ScriptSlider::connectToModulatedParameter(String moduleId, NotUndefined parameterId) -> undefined

Thread safety: UNSAFE
Connects modulation display querying to a processor parameter or modulation target alias.
Also prepares matrix popup support data for valid targets.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.connectToModulatedParameter("SimpleGain1", "Gain");

Dispatch/mechanics:
  property and target resolution route through ScriptSlider matrix bridge setup
  target classification chooses modulator or cable/parameter connection backend

Pair with:
  updateValueFromProcessorConnection -- pulls value refresh from configured target

Anti-patterns:
  - Do NOT pass missing module IDs -- query callback is cleared and no modulation data is shown.

Source:
  ScriptingApiContent.cpp:2054  setScriptObjectPropertyWithChangeMessage(matrixTargetId) -> connection selection
  ScriptModulationMatrix.cpp:1  MatrixCableConnection and MultiMatrixModulatorConnection internals
