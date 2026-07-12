GlobalCable (object)
Obtain via: Engine.getGlobalRoutingManager().getCable(cableId)

Named data bus for routing normalised values and arbitrary data between script
processors and modules. Carries two independent channels: a value channel
(double, 0..1 normalised) and a data channel (serialised JSON/string/buffer).

Complexity tiers:
  1. Basic read/write: getValue, setValue, setValueNormalised. Simple inter-script
     communication or timer-polled value reading from DSP networks.
  2. Callback-driven: + registerCallback with AsyncNotification. Reactive UI
     updates when cable values change.
  3. Data channel: + sendData, registerDataCallback. Structured JSON through cables.
  4. Module integration: + connectToModuleParameter, connectToMacroControl,
     connectToGlobalModulator. Direct module wiring without script middlemen.

Practical defaults:
  - Use AsyncNotification for value callbacks unless you need audio-thread timing.
    Most cable callbacks drive UI updates, which belong on the message thread.
  - Prefer timer-polled getValue() over SyncNotification callbacks for DSP-to-script
    bridging. Avoids realtime-safety requirements and coalesces updates naturally.
  - Use setValueNormalised for cable-to-cable or cable-to-module routing since the
    internal transport is always 0..1. Only use setRange/setValue when endpoints need
    a human-readable range (Hz, dB).
  - For UI component connections via processorId="GlobalCable", set parameterId to
    the cable name string. This is a one-way path: component value changes flow
    into the cable.

Common mistakes:
  - Using a non-realtime-safe function with SyncNotification -- silently ignored,
    callback never fires, no error reported. Must use inline function.
  - Calling sendData() from the audio thread -- allocates a MemoryOutputStream on
    the heap. Move data sending to a timer or async context.
  - Using registerCallback with SyncNotification for UI updates -- repaint() and
    Console.print() are not realtime-safe and will silently fail or cause glitches.
    Use AsyncNotification or timer-polled getValue() instead.
  - Polling getValue() for visual feedback without smoothing -- raw DSP values
    change abruptly between timer ticks. Apply exponential smoothing before rendering.
  - Calling getGlobalRoutingManager() repeatedly -- the routing manager is a
    singleton. Cache the reference once at init time.

Example:
  // Get a global cable and register a callback
  const var rm = Engine.getGlobalRoutingManager();
  const var cable = rm.getCable("MyCable");

  cable.setRange(0.0, 100.0);

  inline function onCableValue(value)
  {
      Console.print("Cable value: " + value);
  };

  cable.registerCallback(onCableValue, AsyncNotification);
  cable.setValue(50.0);

Methods (14):
  connectToGlobalModulator  connectToMacroControl
  connectToModuleParameter  deregisterCallback
  getValue                  getValueNormalised
  registerCallback          registerDataCallback
  sendData                  setRange
  setRangeWithSkew          setRangeWithStep
  setValue                  setValueNormalised
