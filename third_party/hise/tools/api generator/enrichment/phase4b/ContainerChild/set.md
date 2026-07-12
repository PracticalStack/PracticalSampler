ContainerChild::set(String id, NotUndefined newValue) -> undefined

Thread safety: UNSAFE
Sets a component property on the Data tree. Throws a script error if the
property name is not in the valid property list. Equivalent to dot-assignment
syntax (cc.text = "Volume"). Respects the undo manager if useUndoManager is true.
Pair with:
  get -- reads the property that set() writes
Source:
  ScriptingApiContent.cpp  ChildReference::set()
    -> dyncomp::dcid::Helpers::isValidProperty(id) validation
    -> componentData.setProperty(id, newValue, um)
