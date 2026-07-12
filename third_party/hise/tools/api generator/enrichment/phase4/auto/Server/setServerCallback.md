Registers a callback that fires when server activity starts and stops. The callback receives a single boolean parameter: `true` when one or more requests begin processing, and `false` when all pending requests have completed. Use this to drive a loading indicator or spinner in the UI.

The start/stop notification fires once per batch, not once per request - if you queue five requests, the callback fires `true` once at the start and `false` once when all five have finished.

> [!Warning:Does not fire during file downloads] This callback does not fire during file downloads. Downloads have their own progress tracking via the `Server.downloadFile()` callback.
