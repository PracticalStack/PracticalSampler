Kills all active voices and executes the given function on the sample loading thread. The function takes no arguments and has no access to progress or abort checking. Audio processing is suspended (outputting silence) during execution, making this the correct choice for reconfiguring processors - changing bypass states, attributes, or routing.

> [!Warning:Does not trigger finish callback] Does not trigger the finish callback. Use `callOnBackgroundThread()` for tasks that need completion notification.
