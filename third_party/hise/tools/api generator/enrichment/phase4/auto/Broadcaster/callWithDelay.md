Calls a function after a specified delay, independently of the broadcaster's listener system. Listeners registered via `addListener` do not receive anything from this method - it directly invokes the provided function with the given arguments when the timer fires.

Each new call cancels any previously pending delayed function. The `argArray` parameter must be an array, even for a single argument.

> [!Warning:Not a broadcast, timer call only] This does not dispatch through the listener system. It is a standalone timer-based function call, not a broadcast.
