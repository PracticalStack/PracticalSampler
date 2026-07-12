## isOnline

**Examples:**

```javascript:connectivity-gate
// Title: Guard a server request with a connectivity check
// Context: isOnline() blocks for up to 20 seconds when offline, so use it
// sparingly -- only when you need a definitive answer to show the user
// a meaningful "no internet" message before attempting an operation.

Server.setBaseURL("https://forum.hise.audio");

reg connectivityResult = "unknown";

inline function syncWithServer()
{
    if (!Server.isOnline())
    {
        connectivityResult = "offline";
        Console.print("No internet connection - please check your network");
        return;
    }

    connectivityResult = "online";

    Server.callWithGET("api/recent", {}, function(status, response)
    {
        if (status == Server.StatusOK)
            Console.print("Sync complete");
    });
};

syncWithServer();
```
```json:testMetadata:connectivity-gate
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 1000, "expression": "connectivityResult", "value": "online"}
  ]
}
```

**Pitfalls:**
- Do not call `isOnline()` as a routine pre-check before every server request. The request callback already receives `status == 0` on timeout, which is a non-blocking alternative for detecting connectivity issues. Reserve `isOnline()` for situations where you need a definitive answer before proceeding, such as showing an error panel or triggering a retry flow.
