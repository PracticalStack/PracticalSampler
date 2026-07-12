GlobalRoutingManager (object)
Obtain via: Engine.getGlobalRoutingManager()

Singleton factory for named GlobalCable instances with bidirectional OSC
send/receive and per-event-ID data storage. Central access point for the
global routing system -- creates cables, manages OSC connections, and
provides a hash-table storage for attaching numeric data to MIDI events.

Complexity tiers:
  1. Simple value bridge: getCable + GlobalCable.getValue/registerCallback.
     Read DSP network output in script for UI feedback.
  2. Bidirectional parameter control: getCable +
     GlobalCable.setValueNormalised/registerCallback. Two-way communication
     between UI controls and DSP parameters.
  3. Global modulator integration: getCable +
     GlobalCable.connectToGlobalModulator. Route modulators through the cable
     system into a modulation matrix.
  4. OSC integration: + connectToOSC, addOSCCallback, sendOSCMessage. External
     controller communication with OSC-addressable cable IDs (starting with /).

Practical defaults:
  - Use descriptive cable names that identify what value they carry (e.g.,
    "EnvelopeValue", "PeakLevel"). Reserve /-prefixed names for cables that
    need OSC routing.
  - For DSP network integration, the cable name in script must exactly match
    the processorId property of the GlobalCable node in the scriptnode network.
  - When creating a bank of related cables, store them in an array indexed to
    match the corresponding UI components for clean callback routing.
  - Call Engine.getGlobalRoutingManager() once and reuse the reference -- each
    call creates a new wrapper object.

Common mistakes:
  - Using cable IDs without / prefix and expecting OSC routing -- only cables
    whose IDs begin with / participate in OSC send/receive.
  - Calling sendOSCMessage without first calling connectToOSC with a valid
    TargetPort -- silently returns false with no error.
  - Creating a separate Engine.getGlobalRoutingManager() call for each cable
    -- wasteful allocation. Store the manager in a const var and call getCable
    multiple times on it.
  - Using different cable name strings in script vs. DSP network -- cable
    names are matched by exact string equality. A mismatch silently creates a
    disconnected cable.

Example:
  // Set up global routing manager with OSC
  const var rm = Engine.getGlobalRoutingManager();

  rm.connectToOSC({
      "Domain": "/myPlugin",
      "SourcePort": 9000,
      "TargetPort": 9001
  }, function(error) { Console.print("OSC Error: " + error); });

  const var cable = rm.getCable("/volume");

Methods (7):
  addOSCCallback       connectToOSC        getCable
  getEventData         removeOSCCallback   sendOSCMessage
  setEventData
