RoutingMatrix (object)
Obtain via: Synth.getRoutingMatrix("processorId") or ChildSynth.getRoutingMatrix()

Script handle for managing audio channel routing within a processor. Wraps the
internal MatrixData, exposing primary and send connections across up to 16
channels. Works with any processor implementing RoutableProcessor -- synths,
polyphonic effects, and hardcoded DSP modules.

Constants:
  Channel Info:
    NumInputs = (dynamic)     Number of source channels at construction time
    NumOutputs = (dynamic)    Number of destination channels at construction time

Complexity tiers:
  1. Stereo output selection: addConnection with return value checking. Route a
     stereo signal to one of several output bus pairs.
  2. Multi-output with dynamic remapping: + setNumChannels, clear, loop-based
     addConnection rebuild, Broadcaster attachToRoutingMatrix for UI sync.
  3. Builder API multichannel construction: + removeConnection for channel
     isolation. Programmatic signal flow topologies via Builder API.

Practical defaults:
  - Use clear() followed by addConnection() calls when rebuilding a routing
    configuration. More reliable than selective remove/add.
  - Call setNumChannels() before any multichannel addConnection() calls. It
    both expands source channels and relaxes the stereo constraint.
  - Check the return value of addConnection() when routing to higher output
    pairs (3/4, 5/6) -- the host may not support that many channels.
  - Use Synth.getRoutingMatrix("Name") for runtime changes and
    builder.get(proc, builder.InterfaceTypes.RoutingMatrix) during Builder API
    construction. Both return the same RoutingMatrix type.

Common mistakes:
  - Calling addConnection(0, 4) without setNumChannels() first -- the default
    stereo constraint (numAllowedConnections == 2) auto-removes connections
    when you add a third.
  - Reading rm.NumInputs after setNumChannels(8) expecting 8 -- NumInputs and
    NumOutputs are snapshot constants from construction time. Use
    getNumSourceChannels() for the live value.
  - Not checking addConnection() return value for higher channel pairs -- the
    host DAW may not support the requested output channels.
  - Rebuilding routing by adding connections without clearing first -- old
    connections persist and create unexpected signal routing.
  - Calling setNumChannels() on a non-resizeable matrix -- most processors
    have resizeAllowed = false. Throws "Can't resize this matrix".

Example:
  // Get the routing matrix for a synth module
  const var rm = Synth.getRoutingMatrix("SynthName");

  // Route source channel 0 to destination channel 2
  rm.addConnection(0, 2);

  // Query current connections
  Console.print(rm.getNumSourceChannels());
  Console.print(rm.getDestinationChannelForSource(0));

Methods (11):
  addConnection                 addSendConnection
  clear                         getDestinationChannelForSource
  getNumDestinationChannels     getNumSourceChannels
  getSourceChannelsForDestination  getSourceGainValue
  removeConnection              removeSendConnection
  setNumChannels
