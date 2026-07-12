Attaches a scripted look and feel object to this component and propagates it to all ScriptComponent children. Pass `false` to clear it.

> [!Warning:Does not affect dynamic children] The look and feel only propagates to regular ScriptComponent children (those with `parentComponent` set to this container). Dynamic children created via `setData()` are not affected - they use their own CSS styling through `class` and `elementStyle` properties.
