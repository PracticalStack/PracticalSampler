Content::componentExists(String name) -> bool

Thread safety: SAFE
Checks whether a component with the given name exists on the interface. Returns true
if any addXXX() method has created a component with that name, false otherwise. Safe to
call at any time (not restricted to onInit).

Pair with:
  getComponent -- retrieve the component reference if it exists
  getAllComponents -- batch-retrieve components by regex pattern

Source:
  ScriptingApiContent.cpp  Content::componentExists()
    -> searches components array by name, returns bool
