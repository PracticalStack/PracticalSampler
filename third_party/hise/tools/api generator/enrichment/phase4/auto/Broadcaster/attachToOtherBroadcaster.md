Chains this broadcaster to one or more source broadcasters so that messages from the source(s) are forwarded to this broadcaster's listeners. Use the optional `argTransformFunction` to reshape arguments between broadcasters with different structures - the function must return an array matching this broadcaster's argument count. Pass `false` to forward arguments unchanged.

The `async` parameter controls whether forwarded messages dispatch synchronously or asynchronously. On attachment, existing listeners immediately receive the source broadcaster's current values.

> [!Warning:Transform function must return an array] The transform function must return an array. Returning a scalar value causes the original source arguments to be forwarded unchanged, which may trigger an argument count mismatch if the two broadcasters have different argument counts.
