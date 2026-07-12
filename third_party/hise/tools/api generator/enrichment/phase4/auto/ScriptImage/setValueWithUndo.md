Sets the value through the undo manager, allowing the change to be reverted. Use this for user-initiated value changes that should support undo/redo.

> [!Warning:Avoid calling from control callbacks] Do not call this from `onControl` or a custom control callback. The undo operation itself triggers the control callback, creating a feedback loop.
