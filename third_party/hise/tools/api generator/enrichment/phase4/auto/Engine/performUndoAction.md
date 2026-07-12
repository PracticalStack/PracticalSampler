Registers and immediately performs a scriptable undo action. The callback receives `false` when the action is performed and `true` when it is undone via `Engine.undo()`. The first parameter is bound as the `this` context inside the callback, allowing you to store old and new state for reversal. Clone any mutable state (arrays, objects) before passing it as the old value, or the reference will track the current value rather than preserving the original.

```js
Engine.performUndoAction({
    "obj": myList,
    "newValue": [3, 4, 5],
    "oldValue": myList.clone()
}, function(isUndo) {
    this.obj.clear();
    for (v in isUndo ? this.oldValue : this.newValue)
        this.obj.push(v);
});
```