## setBaseURL

**Examples:**

```javascript:server-initialization
// Title: Initialize the server and verify with a GET request
// Context: setBaseURL starts the background WebThread. Call it once during
// onInit before any other Server method. All subsequent calls use relative
// sub-URLs appended to this base.

Server.setBaseURL("https://forum.hise.audio");

reg serverReady = false;

// Verify the server thread is running with a simple GET request
Server.callWithGET("api/user/christoph-hart", {}, function(status, response)
{
    if (status == Server.StatusOK)
        serverReady = true;
});
```
```json:testMetadata:server-initialization
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 1000, "expression": "serverReady", "value": true}
  ]
}
```
