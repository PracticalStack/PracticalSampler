Sends a content change notification to all registered listeners and content callbacks, even if the content has not actually changed. Use this after modifying audio data directly through Buffer objects obtained from `getContent()` to trigger UI refreshes and listener updates.

> [!Warning:Notification fires asynchronously] The notification is asynchronous - callbacks will not have fired by the time `update()` returns.
