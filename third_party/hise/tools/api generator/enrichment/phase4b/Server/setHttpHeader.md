Server::setHttpHeader(String additionalHeader) -> undefined

Thread safety: UNSAFE -- assigns a String to the GlobalServer's extraHeader field. String assignment involves atomic ref-count operations and potential memory management.
Sets the HTTP header string applied to all subsequent GET, POST, and download requests. Replaces any previously set header (not appended). Each request copies the current header when queued, so changing the header after queuing does not affect pending requests. Pass empty string to clear. Multiple headers can be separated with \r\n. Persists across script recompilations.
Anti-patterns:
  - [BUG] Passing a complex JSON object (nested arrays/objects) as the parameters argument to callWithGET() or callWithPOST() silently overwrites the header with "Content-Type: application/json". Call setHttpHeader() again after such a request to restore the desired header.
  - The header is global -- it applies to all request types (GET, POST, downloads). There is no per-request header API.
Source:
  GlobalServer.cpp  GlobalServer::setHttpHeader()
    -> extraHeader = newHeader
    -> header copied into PendingCallback.extraHeader when request is added to queue
    -> also passed to ScriptDownloadObject on creation
