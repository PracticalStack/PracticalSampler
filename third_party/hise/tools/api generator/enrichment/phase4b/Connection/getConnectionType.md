Connection::getConnectionType() -> Integer

Thread safety: SAFE
Returns an integer indicating the connection source type: MacroParameter (0),
SingleOutputModulation (1), or MultiOutputModulation (2).

Anti-patterns:
  - [BUG] The underlying `type` member is never assigned in the constructor or
    elsewhere. The return value is uninitialized. Do NOT rely on this method for
    connection type discrimination. Use getSourceNode() to inspect the source
    node type instead.

Source:
  NodeBase.h:713  ConnectionBase class -- ConnectionSource enum defined but never assigned
  NodeBase.cpp:1632  Constructor does not set `type` member
