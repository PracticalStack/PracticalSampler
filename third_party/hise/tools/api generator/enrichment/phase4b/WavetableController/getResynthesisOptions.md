WavetableController::getResynthesisOptions() -> JSON

Thread safety: UNSAFE -- constructs a DynamicObject with string property names via toVar()
Returns the current resynthesis options as a JSON object. Modify properties on
the returned object and pass it back to setResynthesisOptions to update settings.

Pair with:
  setResynthesisOptions -- modify the returned object and pass it back to update settings

Source:
  ScriptingApiObjects.cpp:5215  getResynthesisOptions()
    -> wt->getResynthesisOptions().toVar() constructs DynamicObject from ResynthesisOptions
