Controls the bypass state of the broadcaster. When bypassed, incoming values are still stored but listener callbacks are not invoked. Set `sendMessageIfEnabled` to `true` when unbypassing to resend the last stored values, synchronising listeners with any changes that arrived during the bypass period.

The `bypass()` scoped statement provides a RAII alternative: the broadcaster is bypassed for the duration of the block and automatically restored on exit.

> [!Warning:Third parameter async semantics inverted] The third parameter (`async`) has inverted semantics relative to its name. Passing `true` causes synchronous dispatch, and `false` causes asynchronous dispatch. Use `SyncNotification` or `AsyncNotification` constants for clarity.
