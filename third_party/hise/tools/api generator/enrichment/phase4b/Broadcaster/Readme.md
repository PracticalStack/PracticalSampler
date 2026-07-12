Broadcaster (object)
Obtain via: Engine.createBroadcaster(defaultValues)

Structured event system for multi-source, multi-target message passing with
metadata, priority sorting, and debug visualization. Connects event sources
(component changes, module parameters, mouse events, complex data, etc.) to
targets (callbacks, component setters, module parameter syncers, other
broadcasters). Messages carry a fixed number of named arguments defined at
creation time. Dot-assignment (bc.argName = value) triggers synchronous sends;
function-call syntax bc(args) sends sync when no sources are attached, async
otherwise.

Complexity tiers:
  1. Single-purpose broadcaster: Engine.createBroadcaster, one attachTo* source,
     addListener. Simple radio group switching, component value monitoring,
     context menus.
  2. Multi-target reactive binding: + addComponentPropertyListener,
     addComponentRefreshListener, addComponentValueListener. Parameter-to-UI
     synchronization and visual state management.
  3. Coordinated broadcaster patterns: + attachToOtherBroadcaster, setBypassed,
     addDelayedListener, setRealtimeMode, setEnableQueue, priority metadata.
     Cross-broadcaster communication, drag-bypass pairs, preset lifecycle
     chains, centralized event bus architectures.

Practical defaults:
  - Use attachToRadioGroup for page navigation -- handles button state tracking
    and provides zero-based selected index directly.
  - Use addDelayedListener with 30-100ms delay for debouncing rapid UI changes
    (search fields, tag filters, preset browser toggling).
  - Use { "id": "name", "args": ["a", "b"] } creation format for readable code
    and BroadcasterMap debugger support.
  - Set priority in metadata to control listener execution order. Higher values
    execute first. State-setting listeners at 100, UI-update listeners lower.
  - Pass false (not a function) as optionalFunction in addComponentPropertyListener
    and addComponentValueListener for direct mode without transformation.

Common mistakes:
  - Creating a broadcaster with wrong arg count for the intended attachTo* method
    (e.g., 3 args then calling attachToComponentValue which needs 2) -- throws a
    script error at attachment time.
  - Mutating an object in-place and resending, expecting change detection to
    trigger -- var::operator!= is reference-based for objects. Create a new
    object instance or enable queue mode.
  - Sending a message with undefined arguments and expecting callbacks to fire --
    suppressed silently with no error. Call setSendMessageForUndefinedArgs(true)
    if needed.
  - Providing a callback to addComponentPropertyListener or
    addComponentValueListener with wrong arg count or no return value -- callback
    receives targetIndex as an extra first argument (N+1 total) and must return
    the value to set.
  - Adding listeners in many separate scripts that directly call each other --
    define core broadcasters in a central data file and subscribe from feature
    modules for decoupling.
  - Using Content.getComponent() inside a broadcaster callback on every fire --
    cache component references in const var at init time.

Callback slot compatibility:
  Broadcaster objects can be passed directly to any callback slot (e.g.
  ErrorHandler.setErrorCallback(), MIDIPlayer.setPlaybackCallback(),
  ScriptPanel.setFileDropCallback()) in place of a function reference. The
  broadcaster's arg count must match the expected callback parameter count.
  Triggers async listener calls.

Example:
  // Create a broadcaster with two named arguments
  const var bc = Engine.createBroadcaster({
      id: "myBroadcaster",
      args: ["component", "value"]
  });

  // Attach to component value changes as source
  bc.attachToComponentValue(["Knob1", "Knob2"], "knobSource");

  // Register a listener as target
  bc.addListener("", "valueLogger", function(component, value)
  {
      Console.print(component + ": " + value);
  });

Methods (39):
  addComponentPropertyListener    addComponentRefreshListener
  addComponentValueListener       addDelayedListener
  addListener                     addModuleParameterSyncer
  attachToComplexData             attachToComponentMouseEvents
  attachToComponentProperties     attachToComponentValue
  attachToComponentVisibility     attachToContextMenu
  attachToEqEvents                attachToInterfaceSize
  attachToModuleParameter         attachToNonRealtimeChange
  attachToOtherBroadcaster        attachToProcessingSpecs
  attachToRadioGroup              attachToRoutingMatrix
  attachToSampleMap               callWithDelay
  isBypassed                      refreshContextMenuState
  removeAllListeners              removeAllSources
  removeListener                  removeSource
  resendLastMessage               reset
  sendAsyncMessage                sendMessageWithDelay
  sendSyncMessage                 setBypassed
  setEnableQueue                  setForceSynchronousExecution
  setRealtimeMode                 setReplaceThisReference
  setSendMessageForUndefinedArgs
