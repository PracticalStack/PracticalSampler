RoutingMatrix::getDestinationChannelForSource(var sourceIndex) -> Integer or Array

Thread safety: WARNING -- no lock for single-value queries (reads fixed-size array). Array input mode constructs a new Array (heap allocation).
Returns the destination channel index that the given source is connected to via primary connection.
Returns -1 if the source is not connected. Accepts a single index or an array of indices; array input returns an array of results.
Dispatch/mechanics:
  Single index: reads channelConnections[source] directly (no lock)
  Array input: iterates elements, recursive per-element call, allocates result Array
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::getDestinationChannelForSource()
    -> reads MatrixData::channelConnections[] directly
