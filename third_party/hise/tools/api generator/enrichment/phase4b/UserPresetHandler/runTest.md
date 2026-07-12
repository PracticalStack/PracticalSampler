UserPresetHandler::runTest() -> undefined

Thread safety: WARNING -- extensive String construction, ValueTree export, XML generation, JSON serialization; outputs to console
Runs diagnostic checks for common preset data persistency issues and prints
results to the HISE console. Checks: stats summary, connected component issues,
custom data round-trip consistency (save-load-save compare), module state dumps.
Development-time tool only.
Source:
  ScriptExpansion.cpp:817  runTest()
    -> reports isCustomModel, numSaveInPreset, totalComponents, automationSlots
    -> checks connected components for saveInPreset/moduleState conflicts
    -> if custom model: save -> load -> save -> compare JSON strings
    -> dumps module state ValueTrees as XML
