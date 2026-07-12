Adds a target that synchronises a module parameter from the broadcaster's last argument value. When the broadcaster fires, the final argument is applied to the specified parameter on the target module. The `parameterIndex` can be a string parameter name or an integer index.

> [!Warning:Enables synchronous execution for broadcaster] This method forces the entire broadcaster into synchronous execution mode. All subsequent sends - including `sendAsyncMessage` - execute synchronously. This is a broadcaster-level side effect, not scoped to this syncer.
