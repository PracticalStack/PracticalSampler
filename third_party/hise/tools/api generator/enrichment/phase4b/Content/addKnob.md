Content::addKnob(String knobName, int x, int y) -> ScriptSlider

Thread safety: INIT -- calling after onInit throws a script error.
Creates a ScriptSlider component (rotary knob or linear slider, depending on the mode
property). Despite the method name, the returned type is ScriptSlider -- "addKnob" reflects
the default rotary appearance. Accepts 1 arg (name only) or 3 args (name, x, y).
Idempotent: if a component with the same name exists, returns the existing one with
optional position update.

Pair with:
  getComponent -- retrieve the component reference after creation
  componentExists -- check if a component already exists before creating
  setUpdateExistingPosition -- control whether re-calls update x/y

Anti-patterns:
  - Do NOT call after onInit -- throws "Tried to add a component after onInit()".

Source:
  ScriptingApiContent.h:~3510  addComponent<ScriptSlider>(name, x, y)
  ScriptingApiContent.cpp:~7781  setMethod("addKnob", ...)
