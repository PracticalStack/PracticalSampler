RoutingMatrix::getNumSourceChannels() -> Integer

Thread safety: SAFE
Returns the current number of source (input) channels in the routing matrix.
Unlike the NumInputs constant (snapshot from construction time), this reflects the live value and updates after setNumChannels() is called.
Pair with:
  getNumDestinationChannels -- companion query for output side
  setNumChannels -- the method that changes the source channel count
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::getNumSourceChannels()
    -> MatrixData::getNumSourceChannels() in Routing.h
