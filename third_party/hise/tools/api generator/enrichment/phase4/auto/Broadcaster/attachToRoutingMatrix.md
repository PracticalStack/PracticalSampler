Attaches the broadcaster to the routing matrix of one or more processors, firing whenever channels are connected or disconnected. The callback receives `(processorId, matrix)` where `matrix` is a `ScriptRoutingMatrix` object for querying the routing configuration.

Queue mode is automatically enabled. On attachment, existing listeners immediately receive the current routing state.

> [!Warning:Target must have routing matrix] The target module must have a routing matrix. Effects and sound generators have routing matrices; modulators do not. Passing a modulator ID produces an error.

> [!Warning:Avoid recursive routing matrix changes] Do not call functions in the listener callback that themselves change the routing matrix, or you will create an infinite loop.
