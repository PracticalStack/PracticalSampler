MidiAutomationHandler (object)
Obtain via: Engine.createMidiAutomationHandler()

Manages MIDI CC-to-parameter automation mappings. Provides read/write access to
automation data as JSON, customization of the right-click CC assignment popup
(CC filtering, display names, exclusive mode), and a change-notification callback
that fires on MIDI learn, preset load, removal, or programmatic data updates.

Complexity tiers:
  1. Basic popup customization: setControllerNumbersInPopup, setControllerNumberNames,
     setExclusiveMode. Restrict which CCs appear in the right-click popup with readable
     names. No callback needed.
  2. Monitoring automation changes: + setUpdateCallback. Receive notifications when CC
     assignments change. Forward data to a Broadcaster for multi-listener UI updates.
  3. Full programmatic management: + getAutomationDataObject, setAutomationDataFromObject.
     Read-modify-write automation entries for batch operations, undo/redo integration,
     and custom automation UIs beyond the built-in popup.

Practical defaults:
  - Enable exclusive mode (setExclusiveMode(true)) when each CC should control only
    one parameter. Prevents accidental multi-assignment and grays out taken CCs.
  - Keep setConsumeAutomatedControllers(true) (the default) unless you need automated
    CC messages to also reach onController callbacks.
  - Forward update callback data to a Broadcaster rather than handling it inline.
    Decouples the automation system from UI code and lets multiple listeners react.
  - Always pair setControllerNumbersInPopup() with setControllerNumberNames() so
    users see readable labels instead of raw CC numbers.

Common mistakes:
  - Passing a non-function value to setUpdateCallback to "clear" it -- silently
    ignored, previous callback stays active. There is no unregister mechanism.
  - Passing a non-Array to setAutomationDataFromObject -- silently clears all
    automation data instead of reporting an error.
  - Calling setAutomationDataFromObject() from inside the update callback --
    causes infinite recursion during synchronous preset loads.
  - Calling setControllerNumbersInPopup() without setControllerNumberNames() --
    popup shows raw "CC#N" labels instead of readable names.
  - Handling all automation-change logic inside setUpdateCallback directly --
    use a Broadcaster for independent UI listeners instead.
  - Removing automation entries without undo support -- wrap
    setAutomationDataFromObject() in Engine.performUndoAction() with
    before/after snapshots.

Example:
  const mah = Engine.createMidiAutomationHandler();

  // Only show Mod Wheel and Expression in the CC popup
  mah.setControllerNumbersInPopup([1, 11]);

  // Give them readable names
  mah.setControllerNumberNames("Controls", ["Mod Wheel", "Expression"]);

  // Get notified when automation mappings change
  mah.setUpdateCallback(function(data)
  {
      Console.print("Automation entries: " + data.length);
  });

Methods (7):
  getAutomationDataObject        setAutomationDataFromObject
  setConsumeAutomatedControllers setControllerNumberNames
  setControllerNumbersInPopup    setExclusiveMode
  setUpdateCallback
