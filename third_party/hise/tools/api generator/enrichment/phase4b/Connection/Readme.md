Connection (object)
Obtain via: Node.connectTo(parameterTarget, sourceInfo) or Parameter.addConnectionFrom(connectionData)

Scriptnode connection handle linking a source node or macro parameter to a target
parameter. Read-only introspection handle for querying source, target, update rate,
and validity of a connection in the DspNetwork graph. Becomes invalid once removed.

Common mistakes:
  - Using getSourceNode(false) to find the signal generator -- returns the immediate
    source which may be a cable node. Use getSourceNode(true) to trace through
    cable intermediaries to the actual signal-producing node.
  - Calling getters on a disconnected Connection without checking isConnected() first
    -- returns undefined/0 silently, no error thrown.

Example:
  // Connect a modulation source to a target parameter
  const var cn = modNode.connectTo(targetParam, 0);

  // Inspect the connection
  Console.print(cn.isConnected());           // true
  Console.print(cn.getUpdateRate());         // block size (e.g. 512)

  var source = cn.getSourceNode(true);       // actual signal source
  var target = cn.getTarget();               // target Parameter object

  // Remove the connection
  cn.disconnect();
  Console.print(cn.isConnected());           // false

Methods (6):
  disconnect          getConnectionType   getSourceNode
  getTarget           getUpdateRate       isConnected
