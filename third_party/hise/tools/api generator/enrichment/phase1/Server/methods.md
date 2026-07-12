## callWithGET

**Signature:** `undefined callWithGET(String subURL, ComplexType parameters, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a PendingCallback object and adds it to the WebThread queue. String construction and reference-counted object management are involved.
**Minimal Example:** `Server.callWithGET("users", {"id": 42}, onServerResponse);`

**Description:**
Sends an asynchronous HTTP GET request to a sub-URL appended to the base URL. The parameters are encoded as URL query parameters (for flat JSON objects) or as JSON POST body (for nested objects/arrays). The callback is invoked on the Server Thread when the response arrives, receiving the HTTP status code and the parsed response. If the response is valid JSON, it is parsed into an object; otherwise the raw response string is passed. If the server does not respond within the timeout (default 10 seconds), the callback receives `Server.StatusNoConnection` (0) and the timeout message string (default `"{}"`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| subURL | String | yes | Path segment appended to the base URL | Must not be empty |
| parameters | ComplexType | yes | Request parameters: flat JSON object for query params, nested JSON for JSON body, or string for raw data | -- |
| callback | Function | yes | Callback invoked when response arrives | Must accept 2 arguments |

**Callback Signature:** callback(status: int, response: var)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| status | Integer | HTTP status code (use Server.StatusOK, Server.StatusNotFound, etc.) |
| response | var | Parsed JSON object if valid JSON, otherwise the raw response string |

**Pitfalls:**
- [BUG] Passing a complex JSON object (containing arrays or nested objects) as `parameters` silently sets the global `Content-Type: application/json` header via `getWithParameters()`, affecting all subsequent requests until `setHttpHeader()` is called again.

**Cross References:**
- `$API.Server.callWithPOST$`
- `$API.Server.setBaseURL$`
- `$API.Server.setHttpHeader$`
- `$API.Server.setServerCallback$`
- `$API.Server.setTimeoutMessageString$`
- `$API.Server.resendLastCall$`

**Example:**
```javascript:get-request-with-response
// Title: GET request with JSON response handling
Server.setBaseURL("https://api.example.com");

inline function onServerResponse(status, response)
{
    if(status == Server.StatusOK)
    {
        Console.print("Name: " + response.name);
    }
    else
    {
        Console.print("Error: " + status);
    }
};

Server.callWithGET("users", {"id": 42}, onServerResponse);
```
```json:testMetadata:get-request-with-response
{
  "testable": false,
  "skipReason": "requires-live-server"
}
```

## callWithPOST

**Signature:** `undefined callWithPOST(String subURL, ComplexType parameters, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a PendingCallback object and adds it to the WebThread queue. String construction and reference-counted object management are involved.
**Minimal Example:** `Server.callWithPOST("login", {"user": "admin", "pass": "1234"}, onLoginResponse);`

**Description:**
Sends an asynchronous HTTP POST request to a sub-URL appended to the base URL. By default, a trailing slash is automatically appended to the sub-URL to prevent HTTP 301 redirects from POST to GET. The trailing slash is NOT added if the sub-URL contains a dot (file endpoint) or already ends with a slash. This behavior can be disabled with `setEnforceTrailingSlash(false)`. The parameters are encoded as URL query parameters (for flat JSON), JSON POST body (for nested JSON with `Content-Type: application/json`), or raw POST data (for strings). The callback is invoked on the Server Thread when the response arrives.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| subURL | String | yes | Path segment appended to the base URL | Must not be empty |
| parameters | ComplexType | yes | Request parameters: flat JSON for query params, nested JSON for JSON body, or string for raw POST data | -- |
| callback | Function | yes | Callback invoked when response arrives | Must accept 2 arguments |

**Callback Signature:** callback(status: int, response: var)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| status | Integer | HTTP status code (use Server.StatusOK, Server.StatusNotFound, etc.) |
| response | var | Parsed JSON object if valid JSON, otherwise the raw response string |

**Pitfalls:**
- POST calls automatically append a trailing slash to the sub-URL by default. This prevents HTTP 301 redirects but may cause 404 errors on servers that do not accept trailing slashes. Use `Server.setEnforceTrailingSlash(false)` to disable this.
- [BUG] Passing a complex JSON object (containing arrays or nested objects) as `parameters` silently sets the global `Content-Type: application/json` header via `getWithParameters()`, affecting all subsequent requests until `setHttpHeader()` is called again.

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.setBaseURL$`
- `$API.Server.setEnforceTrailingSlash$`
- `$API.Server.setHttpHeader$`
- `$API.Server.setServerCallback$`
- `$API.Server.setTimeoutMessageString$`
- `$API.Server.resendLastCall$`

**Example:**
```javascript:post-request-with-json
// Title: POST request with JSON body
Server.setBaseURL("https://api.example.com");

inline function onLoginResponse(status, response)
{
    if(status == Server.StatusOK)
    {
        Console.print("Token: " + response.token);
    }
};

Server.callWithPOST("auth/login", {"username": "admin", "password": "secret"}, onLoginResponse);
```
```json:testMetadata:post-request-with-json
{
  "testable": false,
  "skipReason": "requires-live-server"
}
```

## cleanFinishedDownloads

**Signature:** `undefined cleanFinishedDownloads()`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets an atomic boolean flag only. The actual removal happens asynchronously on the Server Thread.
**Minimal Example:** `Server.cleanFinishedDownloads();`

**Description:**
Signals the Server Thread to remove all finished downloads from the pending downloads list. The removal does not happen immediately -- it sets an atomic flag (`cleanDownloads`) that the Server Thread checks on its next iteration (within 500ms). After cleaning, finished downloads are no longer returned by `getPendingDownloads()`. Downloads that are still in progress are not affected.

**Parameters:**

(none)

**Cross References:**
- `$API.Server.getPendingDownloads$`
- `$API.Server.downloadFile$`

## downloadFile

**Signature:** `ScriptObject downloadFile(String subURL, JSON parameters, ScriptObject targetFile, Function callback)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptDownloadObject, acquires a lock on the download queue, and modifies reference-counted arrays.
**Minimal Example:** `var dl = Server.downloadFile("files/patch.zip", {}, targetFile, onDownloadUpdate);`

**Description:**
Initiates an asynchronous file download from a sub-URL appended to the base URL. Returns a `Download` object that can be used to monitor progress, pause, resume, or abort the download. The `targetFile` must be a `File` object obtained from `FileSystem` -- passing a string path triggers a script error. If the target file already exists and has content, the download automatically resumes using HTTP Range headers. Downloads are deduplicated by URL: calling `downloadFile` twice with the same URL reuses the existing download and updates its callback rather than starting a second download. The download callback receives 0 explicit arguments; inside the callback, `this` refers to the Download object, providing access to progress, status, and control methods. If the sub-URL contains query parameters (e.g., `"file.zip?token=abc"`) and the `parameters` object is empty, the query string is automatically parsed and moved into the parameters object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| subURL | String | yes | Path segment appended to the base URL, optionally with query parameters | Must not be empty |
| parameters | JSON | yes | Request parameters appended as URL query parameters or JSON body | Pass `{}` for no parameters |
| targetFile | ScriptObject | yes | Target file for the download | Must be a File object, not a directory |
| callback | Function | yes | Callback invoked periodically during download and on completion | Must accept 0 arguments; `this` is the Download object |

**Callback Signature:** callback()

**Pitfalls:**
- The `targetFile` parameter must be a `File` object from `FileSystem`, not a string path. Passing a string produces a script error ("target file is not a file object").
- The download callback receives 0 arguments. The Download object is accessible as `this` inside the callback. Use `this.getProgress()`, `this.isRunning()`, etc. to query state.

**Cross References:**
- `$API.Server.downloadFile$`
- `$API.Server.cleanFinishedDownloads$`
- `$API.Server.setNumAllowedDownloads$`
- `$API.Server.setBaseURL$`
- `$API.Server.setHttpHeader$`

**Example:**
```javascript:download-file-with-progress
// Title: Download a file with progress monitoring
Server.setBaseURL("https://files.example.com");

const var targetFile = FileSystem.getFolder(FileSystem.Downloads).getChildFile("patch.zip");

inline function onDownloadProgress()
{
    if(this.data.finished)
    {
        if(this.data.success)
            Console.print("Download complete!");
        else
            Console.print("Download failed");
    }
    else
    {
        Console.print("Progress: " + Math.round(this.getProgress() * 100) + "%");
    }
};

var dl = Server.downloadFile("releases/patch.zip", {}, targetFile, onDownloadProgress);
```
```json:testMetadata:download-file-with-progress
{
  "testable": false,
  "skipReason": "requires-live-server"
}
```

## getPendingCalls

**Signature:** `Array getPendingCalls()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates the pending callbacks list and constructs a new Array with heap allocation.
**Minimal Example:** `var calls = Server.getPendingCalls();`

**Description:**
Returns an array of all pending GET and POST request objects currently queued on the Server Thread. Each element is a PendingCallback object. Completed requests that have already been processed are not included -- only requests still waiting to be sent or currently being processed. The array is a snapshot; it is not updated as requests complete.

**Parameters:**

(none)

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`
- `$API.Server.getPendingDownloads$`

## getPendingDownloads

**Signature:** `Array getPendingDownloads()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates the pending downloads list and constructs a new Array with heap allocation.
**Minimal Example:** `var downloads = Server.getPendingDownloads();`

**Description:**
Returns an array of all Download objects currently tracked by the server, including downloads that are waiting, in progress, paused, or finished. Finished downloads remain in the list until `cleanFinishedDownloads()` is called. Each element is a `Download` object with methods for progress monitoring, pausing, resuming, and aborting. The array is a snapshot; it is not updated as download states change.

**Parameters:**

(none)

**Cross References:**
- `$API.Server.downloadFile$`
- `$API.Server.cleanFinishedDownloads$`
- `$API.Server.getPendingCalls$`

## isEmailAddress

**Signature:** `Integer isEmailAddress(String email)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Delegates to JUCE's URL::isProbablyAnEmailAddress, which performs a simple pattern check with no allocations or I/O.
**Minimal Example:** `var valid = Server.isEmailAddress("user@example.com");`

**Description:**
Checks whether the given string looks like a valid email address. Delegates to JUCE's `URL::isProbablyAnEmailAddress()`, which performs a basic structural check (presence of `@`, domain part, etc.). This is not a comprehensive RFC-compliant validation -- it catches obvious formatting errors but may accept some invalid addresses or reject unusual valid ones. Does not perform DNS lookup or SMTP verification. Useful for client-side form validation before submitting to a server.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| email | String | no | The email address string to validate | -- |

**Cross References:**
None.

## isOnline

**Signature:** `Integer isOnline()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Blocks the scripting thread synchronously while attempting HTTP connections. Can block for up to 20 seconds (10s per URL, two URLs tried).
**Minimal Example:** `var online = Server.isOnline();`

**Description:**
Checks whether the system has internet connectivity by attempting to connect to well-known URLs (Google's 204 endpoint, then Amazon as fallback). Returns `true` as soon as any URL responds. This method **blocks the calling thread** (scripting thread) for up to `HISE_SCRIPT_SERVER_TIMEOUT` (default 10 seconds) per URL -- potentially up to 20 seconds total if both URLs time out. The HiseScript engine timeout is automatically extended by the elapsed time to prevent false script timeout errors. Does not require `setBaseURL()` to be called first.

**Parameters:**

(none)

**Pitfalls:**
- This method blocks synchronously and can take up to 20 seconds to return if there is no internet connection. Avoid calling it repeatedly or in performance-sensitive contexts. Consider caching the result or checking connectivity only when needed (e.g., before a retry flow).

**Cross References:**
- `$API.Server.resendLastCall$`

## resendLastCall

**Signature:** `Integer resendLastCall()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Calls isOnline() internally, which blocks the scripting thread synchronously for up to 20 seconds. Also adds to the pending callbacks array and constructs String objects.
**Minimal Example:** `var success = Server.resendLastCall();`

**Description:**
Re-queues the most recent GET or POST request for processing. First checks internet connectivity via `isOnline()` (blocking), then re-adds the last PendingCallback to the WebThread queue. The "last call" is updated every time `callWithGET()` or `callWithPOST()` is called, so this always refers to the single most recent request. Returns `true` if the request was successfully re-queued, `false` if there is no internet connection, no previous call exists, or the callback has become invalid due to script recompilation. The re-queued request reuses the original URL, parameters, and callback function.

**Parameters:**

(none)

**Pitfalls:**
- This method blocks the calling thread for up to 20 seconds (via the internal `isOnline()` check) before re-queuing the request. Avoid calling it in tight loops or performance-sensitive contexts.
- After script recompilation, the last call's callback becomes invalid (WeakCallbackHolder is no longer valid). `resendLastCall()` returns `false` silently in this case -- the request is not re-sent.
- Only the single most recent GET or POST call is tracked. If multiple calls are made, only the last one can be resent.

**Cross References:**
- `$API.Server.isOnline$`
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`

## setBaseURL

**Signature:** `undefined setBaseURL(String url)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a URL object, stores the start time, and starts the WebThread if not already running. Thread creation involves OS-level allocation.
**Minimal Example:** `Server.setBaseURL("https://api.example.com");`

**Description:**
Sets the base URL for all subsequent server requests (GET, POST, and downloads). All sub-URLs passed to `callWithGET()`, `callWithPOST()`, and `downloadFile()` are appended to this base URL. Calling `setBaseURL()` also starts the internal WebThread that processes requests -- this is the activation point for the entire server subsystem. Without calling `setBaseURL()`, the WebThread never starts and no requests are processed. The base URL persists on the GlobalServer backend, surviving script recompilations. Calling `setBaseURL()` multiple times updates the URL and restarts the thread if needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| url | String | no | The base URL for all server requests | Should be a valid HTTP/HTTPS URL |

**Pitfalls:**
- This method must be called before any `callWithGET()`, `callWithPOST()`, or `downloadFile()` call. Without it, the WebThread is not running and queued requests are not processed. The HISE IDE emits a diagnostic warning if request methods are called before `setBaseURL()`.

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`
- `$API.Server.downloadFile$`

**DiagramRef:** server-architecture

## setEnforceTrailingSlash

**Signature:** `undefined setEnforceTrailingSlash(Integer shouldAddSlash)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a plain boolean field on the GlobalServer. No allocations, no locks.
**Minimal Example:** `Server.setEnforceTrailingSlash(false);`

**Description:**
Controls whether POST requests automatically append a trailing slash to the sub-URL. The default is `true`. When enabled, `callWithPOST()` appends a `/` to the sub-URL to prevent HTTP 301 redirects that convert POST to GET -- a common issue with REST APIs behind reverse proxies. The trailing slash is NOT added if the sub-URL contains a dot (assumed to be a file endpoint like `"upload.php"`) or already ends with a slash. Set to `false` if the target server does not accept trailing slashes and returns 404 errors. This setting persists on the GlobalServer and survives script recompilation. Only affects POST calls -- GET calls are not modified.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldAddSlash | Integer | no | Whether to append trailing slashes to POST URLs. `true` (default) or `false`. | -- |

**Cross References:**
- `$API.Server.callWithPOST$`

## setHttpHeader

**Signature:** `undefined setHttpHeader(String additionalHeader)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Assigns a String to the GlobalServer's extraHeader field. String assignment involves atomic ref-count operations and potential memory management.
**Minimal Example:** `Server.setHttpHeader("Authorization: Bearer mytoken123");`

**Description:**
Sets the HTTP header string applied to all subsequent GET, POST, and download requests. The header string replaces any previously set header -- it is not appended. Each request copies the current header at the time it is queued, so changing the header after queuing a request does not affect that request. The header persists on the GlobalServer and survives script recompilation. Pass an empty string to clear the header. The header string should follow HTTP header format (e.g., `"Authorization: Bearer token"` or `"Content-Type: application/json"`). Multiple headers can be set by separating them with `\r\n`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| additionalHeader | String | yes | The HTTP header string for all subsequent requests | Standard HTTP header format |

**Pitfalls:**
- [BUG] Passing a complex JSON object (containing arrays or nested objects) as the `parameters` argument to `callWithGET()` or `callWithPOST()` silently overwrites the header with `"Content-Type: application/json"`. Call `setHttpHeader()` again after such a request to restore the desired header.
- The header is set globally -- it applies to all request types (GET, POST, downloads). There is no per-request header API.

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`
- `$API.Server.downloadFile$`

## setNumAllowedDownloads

**Signature:** `undefined setNumAllowedDownloads(Integer maxNumberOfParallelDownloads)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a plain integer field on the WebThread. No allocations, no locks.
**Minimal Example:** `Server.setNumAllowedDownloads(3);`

**Description:**
Sets the maximum number of downloads that can run in parallel. The default is 1 (sequential downloads). The WebThread manages the download queue and starts waiting downloads up to this limit. When a download completes, the next waiting download is automatically started. If the limit is reduced below the number of currently active downloads, excess downloads are paused until slots become available. This setting persists on the GlobalServer and survives script recompilation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| maxNumberOfParallelDownloads | Integer | no | Maximum number of concurrent downloads | Should be >= 1 |

**Cross References:**
- `$API.Server.downloadFile$`
- `$API.Server.getPendingDownloads$`

## setServerCallback

**Signature:** `undefined setServerCallback(Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new WeakCallbackHolder object, which involves heap allocation and reference counting.
**Minimal Example:** `Server.setServerCallback(onServerActivity);`

**Description:**
Registers a callback function that is invoked when the server's request queue changes state. The callback fires when the request queue transitions to having 1 item remaining (server busy) or 0 items remaining (server idle). It does NOT fire for every individual queue change -- only when the queue size drops below 2. The callback receives a single boolean argument: `true` when the server is processing a request (1 item in queue), `false` when the server becomes idle (0 items in queue). This is useful for showing/hiding loading indicators. The callback is invoked on the Server Thread. Pass `false` to clear the callback. Download queue changes do NOT trigger this callback -- only GET/POST request queue changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | yes | Callback invoked on server activity state changes | Must accept 1 argument |

**Callback Signature:** callback(isActive: bool)

**Pitfalls:**
- The callback only fires when the queue size is below 2 (0 or 1 items). If multiple requests are queued simultaneously, the callback fires once when the queue drains to 1 item and once when it reaches 0 -- not for each intermediate step. This means the callback does not track individual request completions.
- The callback is NOT triggered by download queue changes. Use the download callback on individual Download objects to track download state.

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`

**Example:**
```javascript:server-activity-indicator
// Title: Show a loading indicator during server activity
Server.setBaseURL("https://api.example.com");

inline function onServerActivity(isActive)
{
    if(isActive)
        Console.print("Server busy...");
    else
        Console.print("Server idle");
};

Server.setServerCallback(onServerActivity);
```
```json:testMetadata:server-activity-indicator
{
  "testable": false,
  "skipReason": "requires-live-server"
}
```

## setTimeoutMessageString

**Signature:** `undefined setTimeoutMessageString(String timeoutMessage)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Wraps the string in a var and assigns it to an atomic-safe field on the WebThread. The var construction is trivially cheap for strings.
**Minimal Example:** `Server.setTimeoutMessageString("timeout");`

**Description:**
Sets the string that is used as the response body when a server request times out (no response within `HISE_SCRIPT_SERVER_TIMEOUT`, default 10 seconds). The default timeout message is `"{}"` (an empty JSON object string), which parses to an empty JSON object in the response callback. The timeout message is passed to the callback's second argument (`response`) alongside a status code of `Server.StatusNoConnection` (0). Changing this to a custom string allows the callback to distinguish between a timeout and a successful empty response. The setting persists on the GlobalServer and survives script recompilation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timeoutMessage | String | no | The string returned as the response body on timeout | -- |

**Cross References:**
- `$API.Server.callWithGET$`
- `$API.Server.callWithPOST$`
