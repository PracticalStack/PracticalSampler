Server (namespace)

HTTP client for async GET/POST requests, file downloads, and server activity
monitoring. Wraps a persistent GlobalServer backend that survives script
recompilation, processing requests on a dedicated background thread
("Server Thread").

Constants:
  StatusCodes:
    StatusNoConnection = 0         No internet connection or request timeout
    StatusOK = 200                 HTTP 200 success
    StatusNotFound = 404           HTTP 404 not found
    StatusServerError = 500        HTTP 500 internal server error
    StatusAuthenticationFail = 403 HTTP 403 forbidden / authentication failure

Complexity tiers:
  1. Basic HTTP requests: setBaseURL, callWithGET or callWithPOST, response callback checking status == 200. Sufficient for version checking, telemetry, simple data fetching.
  2. Validated requests: + isOnline, isEmailAddress, resendLastCall. Multi-status error handling (0, 200, 403, 404, 500). Form validation, connectivity gating, retry logic.
  3. Download management: + downloadFile, setNumAllowedDownloads, getPendingDownloads, cleanFinishedDownloads, setServerCallback. Parallel file downloads with progress tracking for expansion packs or sample installation.

Practical defaults:
  - Call Server.setBaseURL() once during onInit before any other Server method. This starts the WebThread -- without it, requests are silently lost.
  - Always check Server.isOnline() before operations that require user feedback on connectivity failure. Note that isOnline() blocks for up to 20 seconds when offline, so call it only when necessary (e.g., before a retry flow), not on every request.
  - Use callWithPOST for requests that send credentials or modifiable data. The trailing slash enforcement (enabled by default) prevents common 301 redirect issues.
  - Handle at least three status categories in callbacks: 0 (no connection/timeout), 200 (success), and everything else (server error). Use Server.StatusOK etc. constants for readability.

Common mistakes:
  - Calling callWithGET/callWithPOST/downloadFile before setBaseURL() -- WebThread is not running, requests silently never execute. The HISE IDE warns about this.
  - Passing a string path instead of a File object to downloadFile() -- triggers a script error. Use FileSystem to obtain a File object.
  - Callback with wrong argument count -- callWithGET/callWithPOST callbacks must accept 2 args (status, response); downloadFile callback must accept 0 args (use `this` for the Download object).
  - Calling isOnline() on every request -- blocks synchronously for up to 20s when offline. The request callback already receives status == 0 on timeout, which is sufficient for most error handling.
  - Checking only status == 200 in response callbacks -- status 0 means no response (timeout/no internet), non-200 codes are actual server responses with error info in the body. Handle separately.

Example:
  // Basic Server usage
  Server.setBaseURL("https://api.example.com");
  Server.setHttpHeader("Authorization: Bearer mytoken123");

  Server.callWithGET("users", {"id": 42}, function(status, response)
  {
      if(status == Server.StatusOK)
          Console.print(trace(response));
  });

Methods (15):
  callWithGET              callWithPOST
  cleanFinishedDownloads   downloadFile
  getPendingCalls          getPendingDownloads
  isEmailAddress           isOnline
  resendLastCall           setBaseURL
  setEnforceTrailingSlash  setHttpHeader
  setNumAllowedDownloads   setServerCallback
  setTimeoutMessageString
