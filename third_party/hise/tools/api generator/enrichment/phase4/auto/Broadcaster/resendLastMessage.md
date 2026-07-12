Re-dispatches the broadcaster's current values to all listeners, bypassing the change-detection gate. Pass `SyncNotification` for synchronous dispatch or `AsyncNotification` for asynchronous dispatch.

This is commonly used after unbypassing a broadcaster to resynchronise listeners, or when external state has changed and listeners need updating without the values themselves having changed.

> [!Warning:Undefined values suppress resend silently] If any of the current values are `undefined`, the message is silently suppressed despite the forced resend.
