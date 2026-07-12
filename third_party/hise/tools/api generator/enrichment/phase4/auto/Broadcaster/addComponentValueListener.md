Adds a target that sets the value of the specified UI components whenever the broadcaster fires. In direct mode (pass `false` for the callback), the last broadcast argument is used as the value. In callback mode, the callback receives `(targetIndex, ...broadcastArgs)` and must return the value to set.

On registration, target components immediately receive their initial values.

> [!Warning:Callback must always return a value] In callback mode, the function must return a value. Returning nothing (implicit `undefined`) triggers an error for each target component.
