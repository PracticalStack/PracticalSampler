Table::setDisplayCallback(Function displayFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder (heap allocation), increments ref count, registers as source.
Registers a callback that fires whenever the table's ruler/display position changes.
This happens when getTableValueNormalised() is called or when a module queries the
table during audio processing. Only one display callback can be active; calling again
replaces the previous. Pass false to clear.
Callback signature: f(double position)

Pair with:
  getTableValueNormalised -- triggers this callback as a side effect
  getCurrentlyDisplayedIndex -- reads back the last position without triggering callback

Source:
  ScriptingApiObjects.cpp:2098  constructor registers via ADD_TYPED_API_METHOD_1 (VarTypeChecker::Function)
  ScriptingApiObjects.cpp:1547  setCallbackInternal(isDisplay=true, f)
    -> WeakCallbackHolder(sp, this, f, 1) -> incRefCount() -> addAsSource()
