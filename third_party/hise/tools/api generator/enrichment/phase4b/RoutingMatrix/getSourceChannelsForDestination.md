RoutingMatrix::getSourceChannelsForDestination(var destinationIndex) -> Integer or Array

Thread safety: UNSAFE -- may construct a new Array when multiple sources map to the same destination (heap allocation even for single-value input).
Returns the source channel(s) connected to the given destination via primary connections. For a single destination: returns -1 (no source), a single integer (one source), or an array (multiple sources fan-in). Accepts an array of destination indices for batch queries.
Dispatch/mechanics:
  Iterates channelConnections[] to find all sources mapping to the given destination
  Single match: returns int. Multiple matches: allocates and returns Array.
  Array input: recursive per-element call, allocates result Array.
Anti-patterns:
  - Do NOT assume the return type is always an integer -- multiple sources can fan-in to
    the same destination, returning an Array. Always check the type before using as an index.
Source:
  ScriptingApiObjects.cpp  ScriptRoutingMatrix::getSourceChannelsForDestination()
    -> scans MatrixData::channelConnections[] for matching destination values
