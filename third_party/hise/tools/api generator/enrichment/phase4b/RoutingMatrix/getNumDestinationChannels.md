RoutingMatrix::getNumDestinationChannels() -> Integer

Thread safety: SAFE
Returns the current number of destination (output) channels in the routing matrix.
Unlike the NumOutputs constant (snapshot from construction time), this reflects the live value.
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::getNumDestinationChannels()
    -> MatrixData::getNumDestinationChannels() in Routing.h
