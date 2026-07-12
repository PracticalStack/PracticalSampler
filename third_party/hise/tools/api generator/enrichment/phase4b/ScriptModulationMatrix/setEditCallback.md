ScriptModulationMatrix::setEditCallback(ComplexType menuItems, Function editFunction) -> undefined

Thread safety: UNSAFE -- constructs a WeakCallbackHolder, modifies container state, registers broadcaster listener.
Registers custom context menu items and a callback for the "Edit connections"
action on modulation targets. Passing an invalid function or empty menu items
clears the edit callback.
Callback signature: f(int menuIndex, String targetId)

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  Stores menu items in container->customEditCallbacks StringArray
    -> WeakCallbackHolder connected to container->editCallbackHandler
    -> fires when user selects a menu item on a modulation target

Anti-patterns:
  - Empty strings and duplicates are automatically removed from menuItems. If all
    items collapse to nothing, the callback is cleared instead of registered.

Source:
  ScriptModulationMatrix.cpp  setEditCallback()
    -> WeakCallbackHolder with 2 args (menuIndex, targetId)
    -> container->editCallbackHandler (LambdaBroadcaster<int, String>)
