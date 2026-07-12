## resendLastCall

**Examples:**

```javascript:retry-button
// Title: Retry button for failed server requests
// Context: A "Retry" button that resends the last server call after
// a connectivity failure. Useful in panels where the user may have
// restored their internet connection.

Server.setBaseURL("https://forum.hise.audio");

reg retryResult = "none";

// Make an initial request
Server.callWithGET("api/user/christoph-hart", {}, function(status, response)
{
    if (status == Server.StatusOK)
        retryResult = response.username;
});

const var retryButton = Content.addButton("RetryButton", 10, 10);
retryButton.set("isMomentary", true);
retryButton.set("text", "Retry");
retryButton.set("saveInPreset", false);

inline function onRetryButton(component, value)
{
    if (value)
    {
        // resendLastCall() internally calls isOnline() (blocking),
        // then re-queues the most recent GET or POST request
        local success = Server.resendLastCall();

        if (!success)
            Console.print("Still no connection or no previous request to retry");
    }
};

retryButton.setControlCallback(onRetryButton);
```
```json:testMetadata:retry-button
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 1000, "expression": "retryResult", "value": "Christoph Hart"}
  ]
}
```

**Pitfalls:**
- `resendLastCall()` only tracks the single most recent GET or POST call. If your workflow makes multiple sequential requests, only the last one can be retried. For multi-step flows, consider tracking the step and re-invoking the appropriate function directly rather than relying on `resendLastCall()`.
