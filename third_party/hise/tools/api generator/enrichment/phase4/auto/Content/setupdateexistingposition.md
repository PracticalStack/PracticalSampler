Controls whether `addXXX()` calls update the x/y position of components that already exist. The default is `true`, meaning `Content.addButton("Btn1", newX, newY)` moves an existing "Btn1" to (newX, newY) on every recompile. Set this to `false` when component positions are managed dynamically at runtime (e.g. via layout scripts, `setPropertiesFromJSON`, or `ScriptDynamicContainer`) and should not be reset on recompile. The flag affects all 14 component creation methods (`addKnob`, `addButton`, `addPanel`, `addComboBox`, etc.).

```js
// Call before your addXXX() calls to preserve dynamic layouts
Content.setUpdateExistingPosition(false);
```
