Returns the source channel(s) connected to the given destination via primary connections. Accepts either a single index or an array of indices.

> [!Warning:Return type varies by connection state] The return type varies depending on the connection state: -1 when no source is connected, a single integer when exactly one source maps to the destination, or an array when multiple sources fan in. Always check the type before using the result as an array index.
