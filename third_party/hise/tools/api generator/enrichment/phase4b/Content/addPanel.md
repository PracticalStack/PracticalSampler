Content::addPanel(String panelName, int x, int y) -> ScriptPanel

Thread safety: INIT -- calling after onInit throws a script error.
Creates a ScriptPanel component (drawable panel with custom paint routines, mouse/key
callbacks, animation timers, drag-and-drop, and child component parenting). The most
versatile component type. Accepts 1 arg (name only) or 3 args (name, x, y). Idempotent:
if a component with the same name exists, returns the existing one with optional
position update.

Pair with:
  getComponent -- retrieve the component reference after creation
  componentExists -- check if a component already exists before creating
  setUpdateExistingPosition -- control whether re-calls update x/y
  setUseHighResolutionForPanels -- enable 2x rendering for custom paint routines

Anti-patterns:
  - Do NOT call after onInit -- throws "Tried to add a component after onInit()".

Source:
  ScriptingApiContent.h:~3510  addComponent<ScriptPanel>(name, x, y)
  ScriptingApiContent.cpp:~7781  setMethod("addPanel", ...)
