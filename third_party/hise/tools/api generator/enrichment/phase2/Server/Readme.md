# Server -- Project Context

## Project Context

### Real-World Use Cases
- **Version checking**: The most common use case. A plugin uses `callWithGET` to fetch a JSON version manifest from a static URL, compares it against the running version, and shows an update notification panel if a newer version exists.
- **Content delivery**: A plugin that distributes expansion packs or sample libraries uses `downloadFile` to fetch content archives with progress tracking, `setNumAllowedDownloads` to limit concurrent transfers, and `setServerCallback` to show a global activity indicator during downloads.
- **User feedback and analytics**: A plugin sends anonymous usage data or crash reports via `callWithPOST`, collecting device information from `Engine.getOS()` and `Engine.getProjectInfo()`. The response callback confirms receipt or reports errors.

### Complexity Tiers
1. **Basic HTTP requests** (most common): `setBaseURL`, `callWithGET` or `callWithPOST`, and a response callback that checks `status == 200`. Sufficient for version checking, telemetry, or simple data fetching.
2. **Validated form submission**: Adds `isOnline` for connectivity gating, `isEmailAddress` for input validation, `resendLastCall` for retry, and multi-status error handling (0, 200, 403, 404, 500) with user-facing error messages.
3. **Download management**: Adds `downloadFile`, `setNumAllowedDownloads`, `getPendingDownloads`, `cleanFinishedDownloads`, and `setServerCallback` for managing parallel file downloads with progress tracking. Used for expansion pack delivery or sample installation.

### Practical Defaults
- Call `Server.setBaseURL()` once during `onInit` before any other Server method. This starts the WebThread - without it, requests are silently lost.
- Always check `Server.isOnline()` before operations that require user feedback on connectivity failure. Note that `isOnline()` blocks for up to 20 seconds when offline, so call it only when necessary (e.g., before a retry flow), not on every request.
- Use `callWithPOST` for any request that sends user-submitted data or form fields. The trailing slash enforcement (enabled by default) prevents common 301 redirect issues with REST APIs.
- Handle at least three status categories in callbacks: `0` (no connection/timeout), `200` (success), and everything else (server error). Use the `Server.StatusOK` etc. constants for readability.

### Integration Patterns
- `Server.callWithGET()` -> UI update notification - Fetch a version manifest JSON, compare version numbers against `Engine.getVersion()`, and conditionally show an update panel.
- `Server.callWithPOST()` -> JSON response handling - Submit form data to a REST API and process the structured response in the callback.
- `Server.isOnline()` -> `Server.callWithPOST()` - Connectivity gating: check internet availability before attempting a request, showing a meaningful "no internet" message instead of waiting for a timeout.
- `Server.isEmailAddress()` -> form validation UI - Validate email format before enabling a submit button. Runs client-side without a server round-trip.
- `Server.resendLastCall()` -> retry button - Wire a "Retry" button to resend the last failed request after the user resolves a connectivity issue.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `Server.isOnline()` on every request to guard connectivity | Call `isOnline()` only before retry flows or when you need an explicit "no internet" message | `isOnline()` blocks synchronously for up to 20 seconds when offline. The request callback already receives `status == 0` on timeout, which is sufficient for most error handling. |
| Checking only `status == 200` in the response callback | Handle `status == 0` (timeout/no connection) separately from non-200 status codes | Status 0 means no response was received at all (timeout or no internet). Non-200 codes (403, 404, 406, 500) are actual server responses with potentially useful error messages in the response body. |
| Hardcoding product names and version strings in request parameters | Use `Engine.getProjectInfo().ProjectName`, `Engine.getVersion()`, and `Engine.getOS()` | These methods return the correct values from the project settings, making the request logic reusable across products without code changes. |
