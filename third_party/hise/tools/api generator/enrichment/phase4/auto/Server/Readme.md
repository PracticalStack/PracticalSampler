<!-- Diagram triage:
  - No diagram specifications in source data. No diagrams to render.
-->

# Server

Server provides an asynchronous HTTP client for communicating with a remote server from HISEScript. It supports GET and POST requests, file downloads with progress tracking, and utility methods for connectivity checks and input validation.

All server operations are asynchronous: methods return immediately and execute a callback function when the server responds. Callbacks run on a dedicated server thread in serial order, so they never block the audio or UI thread.

A typical setup calls `Server.setBaseURL()` once during `onInit` to configure the server endpoint and start the background thread. All subsequent calls use relative sub-URLs that are appended to the base:

```js
Server.setBaseURL("https://myserver.example.com");
```

Use GET requests for fetching non-sensitive data (version manifests, public resources) and POST requests for sending sensitive data (credentials, form data). The response in every callback is automatically parsed from JSON, so your server-side endpoint must return valid JSON-formatted text.

For debugging during development, add a `ServerController` floating tile to your scripting workspace. It shows active requests, pending downloads, and server responses in real time.

Response callbacks receive an integer status code and a parsed JSON response object. Use the named constants for readable error handling:

| Constant | Value | Description |
|----------|-------|-------------|
| `Server.StatusNoConnection` | 0 | No internet connection or request timeout |
| `Server.StatusOK` | 200 | HTTP 200 success |
| `Server.StatusAuthenticationFail` | 403 | HTTP 403 forbidden |
| `Server.StatusNotFound` | 404 | HTTP 404 not found |
| `Server.StatusServerError` | 500 | HTTP 500 internal server error |

> [!Tip:Thread starts on setBaseURL, handle status zero] The server thread only starts when you call `Server.setBaseURL()`. Until then, no background resources are used. Status `0` (`StatusNoConnection`) means no response was received (timeout or no internet) - handle it separately from actual server error codes.

## Common Mistakes

- **Use status 0 check not isOnline polling**
  **Wrong:** Calling `Server.isOnline()` before every request to check connectivity.
  **Right:** Let the request callback handle `status == 0` for timeouts; reserve `Server.isOnline()` for explicit connectivity gates (e.g. before showing an activation panel).
  *`isOnline()` blocks for up to 20 seconds when offline. The callback already reports connection failures via status code 0, which is non-blocking.*

- **Handle all error status codes**
  **Wrong:** Checking only `status == 200` in the response callback.
  **Right:** Handle `status == 0` (timeout/no connection) separately from non-200 error codes (403, 404, 500).
  *Status 0 means no response was received at all. Non-200 codes are actual server responses with potentially useful error messages in the response body. Conflating the two leads to misleading messages for the user.*

- **Use Engine properties for version data**
  **Wrong:** Hardcoding product names and version strings in request parameters.
  **Right:** Use `Engine.getProjectInfo().ProjectName`, `Engine.getVersion()`, and `Engine.getOS()` to build request payloads dynamically.
  *These methods return the correct values from the project settings, keeping the request logic reusable across products without code changes.*

- **Store sensitive data encrypted on disk**
  **Wrong:** Storing sensitive server response data (tokens, API keys) to disk as plain JSON.
  **Right:** Use `File.writeEncryptedObject()` to persist any sensitive data received from the server.
  *Plain JSON files are trivially readable. Encrypted objects protect credentials and sensitive data at rest.*
