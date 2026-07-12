Table::setContentCallback(Function contentFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder (heap allocation), increments ref count, registers as source.
Registers a callback that fires whenever the table's content changes -- when points
are added, removed, modified, or when the table is reset or bulk-updated. Only one
content callback can be active; calling again replaces the previous. Pass false to clear.
Callback signature: f(int pointIndex)

Anti-patterns:
  - The pointIndex argument is -1 for bulk operations (reset, setTablePointsFromArray).
    Check for -1 before using it as an array index into getTablePointsAsArray().

Source:
  ScriptingApiObjects.cpp:2098  constructor registers via ADD_TYPED_API_METHOD_1 (VarTypeChecker::Function)
  ScriptingApiObjects.cpp:1547  setCallbackInternal(isDisplay=false, f)
    -> WeakCallbackHolder(sp, this, f, 1) -> incRefCount() -> addAsSource()
