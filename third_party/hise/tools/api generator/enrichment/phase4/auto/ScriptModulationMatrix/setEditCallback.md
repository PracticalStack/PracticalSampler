Registers custom context menu items and a callback for the edit action on modulation targets. Pass an array of menu item labels (or a single string) and a callback that receives the zero-based menu index and the target ID when an item is selected. Inside the callback, use `this.getComponent(targetId)` to retrieve the associated UI component.

Typical use cases:

- Show a matrix component displaying all connections for the target
- Add a "clear all connections" action for a specific component
- Reset intensity values to defaults
