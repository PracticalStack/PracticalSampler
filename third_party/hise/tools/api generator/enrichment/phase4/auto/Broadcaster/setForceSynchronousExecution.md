Forces all message dispatch to execute synchronously, regardless of how the send was initiated. This overrides `sendAsyncMessage` to behave synchronously and bypasses the delay in `sendMessageWithDelay`.

This is automatically enabled by `addModuleParameterSyncer` to ensure parameter changes apply immediately.
