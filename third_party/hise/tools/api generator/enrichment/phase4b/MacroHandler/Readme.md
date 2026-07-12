MacroHandler (object)
Obtain via: Engine.createMacroHandler()

Programmatic read/write access to macro control connections with change
notification callbacks. Wraps the C++ MacroControlBroadcaster system on the
master ModulatorSynthChain and the MacroManager singleton.

Complexity tiers:
  1. Basic macro monitoring: setUpdateCallback. Watch for macro connection
     changes and update UI accordingly.
  2. Read-modify-write: + getMacroDataObject, setMacroDataFromObject.
     Programmatically toggle individual macro-to-parameter connections from
     UI code (e.g., context menu actions).
  3. Full macro management with custom automation: + setExclusiveMode. Combine
     with UserPresetHandler.setCustomAutomation() to route macros to custom
     automation slots. Clear on init with setMacroDataFromObject([]).

Practical defaults:
  - Enable exclusive mode (setExclusiveMode(true)) when each macro slot should
    control exactly one parameter -- expected mode for NKS and DAW automation.
  - Pass a Broadcaster as the update callback to fan out macro change
    notifications to multiple listeners, avoiding tight coupling.
  - Clear all connections at startup with setMacroDataFromObject([]) before the
    preset model restores them -- prevents stale connections on init.
  - Store one const var mh = Engine.createMacroHandler() at init and reuse it.
    Each call registers a new listener; multiple instances waste resources and
    receive duplicate notifications.

Common mistakes:
  - Modifying the array from getMacroDataObject and expecting changes to apply
    -- it is a snapshot; call setMacroDataFromObject(modifiedArray) to write back.
  - Creating a new MacroHandler per UI interaction -- each createMacroHandler()
    registers a new listener on the macro chain, causing duplicate notifications.
  - Passing an inline function to setUpdateCallback and trying to reach multiple
    UI systems -- use a Broadcaster as the callback instead.

Example:
  const var mh = Engine.createMacroHandler();

  mh.setUpdateCallback(function(macroData)
  {
      Console.print("Macro connections changed: " + macroData.length + " connections");
  });

Methods (4):
  getMacroDataObject      setExclusiveMode
  setMacroDataFromObject  setUpdateCallback
