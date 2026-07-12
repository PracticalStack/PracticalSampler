DspNetwork::setForwardControlsToParameters(Integer shouldForward) -> undefined

Thread safety: SAFE -- sets a boolean flag. No allocations, no locks.
Controls whether UI control values are forwarded directly to the network's root node
parameters (for DAW automation) or routed through regular script callbacks. Default
is true. When enabled, the NetworkParameterHandler bridges root parameters to the
host's parameter system.
Pair with:
  setParameterDataFromJSON -- batch parameter updates (works regardless of forwarding)
Anti-patterns:
  - Do NOT disable without a clear reason -- breaks DAW automation and requires
    manual parameter bridging through script callbacks.
Source:
  DspNetwork.cpp  setForwardControlsToParameters()
    -> sets forwardControls boolean flag
    -> Holder::getCurrentNetworkParameterHandler() checks this flag to decide
       whether to return networkParameterHandler or content's parameter handler
