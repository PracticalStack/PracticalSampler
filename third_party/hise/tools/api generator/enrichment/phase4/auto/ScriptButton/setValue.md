Sets the button's value to 0 (off) or 1 (on). The UI updates asynchronously, so this is safe to call from any thread. Call `changed()` afterwards if you need the control callback to fire.

> [!Warning:Value not restored after recompile] If called during `onInit`, the value will not be restored after recompilation. Set default values via the `defaultValue` property instead.