RoutingMatrix::setNumChannels(Number numSourceChannels) -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock; triggers processor numSourceChannelsChanged() callback and sendChangeMessage.
Sets the number of source channels and the maximum number of allowed connections. Internally sets both numSourceChannels and numAllowedConnections to the provided value, relaxing the default stereo constraint for multichannel routing.
Required setup:
  const var rm = Synth.getRoutingMatrix("SynthName");
  // Processor must have resizeAllowed == true
Dispatch/mechanics:
  Validates 0-16 range (NUM_MAX_CHANNELS)
    -> checks resizingIsAllowed() (throws "Can't resize this matrix" if false)
    -> MatrixData::setNumSourceChannels(n) + setNumAllowedConnections(n)
    -> processor numSourceChannelsChanged() callback
Pair with:
  clear -- typically called after setNumChannels to rebuild connections
  addConnection -- add multichannel connections after expanding
  getNumSourceChannels -- read back the live channel count (not NumInputs constant)
Anti-patterns:
  - Do NOT call on processors that don't allow resizing -- most processors have
    resizeAllowed = false by default. Throws "Can't resize this matrix".
  - Do NOT read NumInputs/NumOutputs expecting updated values -- these are snapshot
    constants from construction time. Use getNumSourceChannels() instead.
  - Do NOT assume destination channels change -- setNumChannels only affects source
    channels and numAllowedConnections, not numDestinationChannels.
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::setNumChannels()
    -> validates isPositiveAndBelow(n, NUM_MAX_CHANNELS+1)
    -> checks resizingIsAllowed()
    -> MatrixData::setNumSourceChannels(n) + setNumAllowedConnections(n) in Routing.cpp
