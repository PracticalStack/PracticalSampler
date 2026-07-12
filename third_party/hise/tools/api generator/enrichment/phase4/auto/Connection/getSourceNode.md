Returns the source node of this connection. The `getSignalSource` parameter controls whether cable intermediaries (such as `routing.local_cable` or `routing.global_cable`) are traced through to find the actual signal-producing node.

- Pass `true` to trace through cable nodes and return the real signal source.
- Pass `false` to return the immediate source node, which may be a cable routing node.

> [!Warning:Returns undefined when disconnected] Returns undefined (not null) when the connection has been removed. Check `isConnected()` before calling this method if you need to distinguish a disconnected connection from a genuine lookup failure.