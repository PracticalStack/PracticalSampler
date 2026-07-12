Sets the component's value. Thread-safe - can be called from any callback. The UI update happens asynchronously.

> [!Warning:Value not restored after recompile] If called during `onInit`, the value will not be restored after recompilation.
