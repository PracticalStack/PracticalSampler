ScriptedViewport::setEventTypesForValueCallback(Array eventTypeList) -> undefined

Thread safety: UNSAFE
Specifies which table event types trigger the parent component's setValue() callback. This enables undo support and value propagation through the normal component value path. Requires table mode.

| Event type | Description |
|------------|-------------|
| `Selection` | Fires when the row selection changes |
| `SingleClick` | Fires on a single click |
| `DoubleClick` | Fires on a double click |
| `ReturnKey` | Fires when the user confirms with Return |
| `SpaceKey` | Fires when the user triggers the focused row with Space |

Defaults: SingleClick, DoubleClick, ReturnKey, SpaceKey.

Stored value shape:
  - row index in normal table mode
  - `[columnIndex, rowIndex]` array in MultiColumnMode

Required setup:
  const var vp = Content.getComponent("ViewportId");
  vp.setTableMode({});
  vp.setEventTypesForValueCallback(["Selection", "DoubleClick"]);
Dispatch/mechanics: Iterates event type strings, maps to EventType enum, sets flags on table model's value callback mask.
Pair with: setTableMode (must be called first), setTableCallback (registers the interaction handler), setValue (triggered by the configured events)
Anti-patterns: Must call setTableMode() first -- reports a script error otherwise. Passing unsupported event names reports a script error. If you also use setTableCallback(), value-triggered updates appear there as `SetValue` or `Undo` events.
Source:
  ScriptTableListModel.cpp  setEventTypesForValueCallback() -> validates event types -> sets valueCallbackFlags
