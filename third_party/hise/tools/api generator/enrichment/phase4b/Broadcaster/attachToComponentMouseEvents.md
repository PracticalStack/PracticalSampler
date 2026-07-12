Broadcaster::attachToComponentMouseEvents(var componentIds, var callbackLevel, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires on mouse events on specified components. Broadcaster must have
2 args (component, event). Valid levels: "No Callbacks", "Context Menu", "Clicks Only",
"Clicks & Hover", "Clicks, Hover & Dragging", "All Callbacks".
Sets forceSend = true (every event dispatched, no change detection).
event JSON object is identical to ScriptPanel.setMouseCallback() format.
Does not override default mouse behaviour -- additive callback.
Dispatch/mechanics:
  Creates InternalMouseListener per component via attachMouseListener().
  Sets forceSend = true (bypasses change detection for every event).
  No initial values dispatched (getNumInitialCalls() = 0).
Pair with:
  addListener -- to handle (component, event) callbacks
  attachToContextMenu -- for context menu-specific mouse handling
Anti-patterns:
  - Broadcaster must have exactly 2 args.
  - Callback level string must match exactly (spaces, ampersand).
  - callbackLevel is converted via toString() -- non-string types fail to match.
Source:
  ScriptBroadcaster.cpp  MouseEventListener constructor
