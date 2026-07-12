# Server -- Class Analysis

## Brief
HTTP client namespace for async GET/POST requests, file downloads, and server activity monitoring.

## Purpose
Server is a global namespace-style API class providing HTTP communication from HiseScript. It wraps a persistent `GlobalServer` backend that survives script recompilation, processing GET/POST requests and file downloads on a dedicated background thread ("Server Thread"). Requests are queued asynchronously and their callbacks receive status codes and parsed JSON responses. The download subsystem supports parallel downloads with configurable concurrency, automatic resume via HTTP Range headers, and deduplication by URL.

## Details

### Architecture

Server is a thin scripting wrapper around `GlobalServer`, which is owned by `MainController -> JavascriptThreadPool`. The GlobalServer persists across script recompilations -- queued requests and downloads survive recompiles, though callbacks from previous compilations become invalid (WeakCallbackHolder pattern).

The system activates when `setBaseURL()` is called, which starts the internal WebThread. Without calling `setBaseURL()`, no requests are processed. The HISE IDE diagnostics warn if GET/POST/download calls are made before `setBaseURL()`.

### State Machine

The server has four lifecycle states:

| State | Condition |
|-------|-----------|
| Inactive | WebThread not running (before `setBaseURL()`) |
| Pause | WebThread running but paused (after stop) |
| Idle | WebThread running, no pending requests |
| WaitingForResponse | WebThread running, requests in queue |

### Request Processing

GET and POST requests are queued as `PendingCallback` objects and processed sequentially on the Server Thread. Each request:
1. Creates a `WebInputStream` from the URL
2. Reads the full response (timeout: 10s default, configurable via `HISE_SCRIPT_SERVER_TIMEOUT`)
3. Parses the response as JSON; falls back to raw string if parsing fails
4. Calls the script callback with `(status, response)` as arguments

### URL Parameter Handling

The `parameters` argument to GET/POST supports three modes:
- **Simple JSON object** (flat key-value) -- appended as URL query parameters
- **Complex JSON object** (nested arrays/objects) -- sent as JSON POST body with `Content-Type: application/json`
- **String** -- sent as raw POST data

See `callWithGET()` and `callWithPOST()` for full parameter encoding details and pitfalls.

### POST Trailing Slash

POST calls automatically append a trailing slash to prevent HTTP 301 redirects. See `setEnforceTrailingSlash()` for details and how to disable this.

### Download System

Downloads run on the same Server Thread but are managed separately from GET/POST calls. See `downloadFile()` for the full download API including deduplication, resume via HTTP Range headers, and callback conventions. See `setNumAllowedDownloads()` for parallel download configuration and `cleanFinishedDownloads()` for lifecycle management.

### Frontend Initialization

In frontend (exported plugin) builds, the server thread buffers requests until the host application finishes loading. Calls queued during initialization are processed once the frontend signals readiness.

## obtainedVia
Global namespace -- access methods directly as `Server.methodName()`.

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| StatusNoConnection | 0 | int | No internet connection or request timeout | StatusCodes |
| StatusOK | 200 | int | HTTP 200 success | StatusCodes |
| StatusNotFound | 404 | int | HTTP 404 not found | StatusCodes |
| StatusServerError | 500 | int | HTTP 500 internal server error | StatusCodes |
| StatusAuthenticationFail | 403 | int | HTTP 403 forbidden / authentication failure | StatusCodes |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Server.callWithGET("endpoint", {}, callback)` without calling `setBaseURL` first | Call `Server.setBaseURL("https://api.example.com")` before any request | The WebThread is not started until `setBaseURL()` is called. Requests made before this are silently lost. |
| `Server.downloadFile("file.zip", {}, "path/to/file.zip", callback)` | `Server.downloadFile("file.zip", {}, FileSystem.getFile("path/to/file.zip"), callback)` | The `targetFile` parameter must be a File object from `FileSystem`, not a string path. Passing a string triggers a script error. |
| `Server.callWithGET("endpoint", {}, function(status) { ... })` | `Server.callWithGET("endpoint", {}, function(status, response) { ... })` | The callback must accept exactly 2 parameters (status code and response). Wrong parameter count is caught by IDE diagnostics. |

## codeExample
```javascript
// Basic Server usage
Server.setBaseURL("https://api.example.com");
Server.setHttpHeader("Authorization: Bearer mytoken123");

Server.callWithGET("users", {"id": 42}, function(status, response)
{
    if(status == Server.StatusOK)
        Console.print(trace(response));
});
```

## Alternatives
- `Download` -- Server.downloadFile() returns a Download handle; Server initiates requests while Download tracks the state of a single transfer.

## Related Preprocessors
- `HISE_SCRIPT_SERVER_TIMEOUT` -- compile-time HTTP timeout in ms (default: 10000)
- `HISE_INCLUDE_PROFILING_TOOLKIT` -- enables server request profiling data collection
- `USE_BACKEND` -- enables IDE diagnostic checks (setBaseURL validation, callback arg count)

## Diagrams

### server-architecture
- **Brief:** Server Request Pipeline
- **Type:** topology
- **Description:** HiseScript calls (callWithGET, callWithPOST, downloadFile) queue PendingCallback or ScriptDownloadObject into the GlobalServer's WebThread. The WebThread processes callbacks sequentially and downloads in parallel (up to numMaxDownloads). Responses are parsed as JSON and delivered back to script callbacks via WeakCallbackHolder. The GlobalServer persists across script recompilations on the JavascriptThreadPool, owned by MainController.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The existing C++ diagnostics already cover the two key precondition issues -- setBaseURL must be called before any request, and callback argument counts are validated. No additional parse-time diagnostics are needed beyond what is already implemented via ADD_CALLBACK_DIAGNOSTIC_RAW.
