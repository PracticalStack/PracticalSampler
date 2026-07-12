Broadcaster::attachToContextMenu(var componentIds, var stateFunction, var itemList, var optionalMetadata, var useLeftClick) -> undefined

Thread safety: INIT -- runtime calls throw script error
Creates popup context menu on components. Broadcaster must have 2 args (component,
menuItemIndex). State function controls ticks, enabled state, and dynamic text per item.
Pass false for stateFunction to skip state management. Auto-enables queue mode.
itemList supports pseudo-markdown: **Bold** for headers, ___ for separators, ~~text~~ for always-disabled.
{DYNAMIC} wildcard in item text triggers stateFunction with type='text' to get dynamic text.
Uses component's LookAndFeel customization for popup rendering.
Callback signature: stateFunction(String type, int index)
Dispatch/mechanics:
  Creates popup context menu on components. Attaches PopupMenuOnly mouse listener.
  State function cached at init, refreshed after selection.
  Auto-enables queue mode. No initial values (getNumInitialCalls() = 0).
Pair with:
  refreshContextMenuState -- manually refresh cached menu item states
  attachToComponentMouseEvents -- for general mouse events
Anti-patterns:
  - State function results are cached -- call refreshContextMenuState when external state changes.
  - useLeftClick is cast to bool from var directly.
  - Disable enableMidiLearn on attached components to prevent MIDI learn popup conflict.
Source:
  ScriptBroadcaster.cpp  ContextMenuListener constructor
