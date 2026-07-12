Sends a message asynchronously, dispatching listener callbacks on the scripting thread. Without queue mode, rapid consecutive async sends are coalesced - only the most recent values are dispatched when the job executes. Enable queue mode with `setEnableQueue(true)` if every value transition must be observed.

> [!Warning:Undefined arguments suppress message silently] If any argument is `undefined`, the message is silently suppressed with no callbacks and no error.
