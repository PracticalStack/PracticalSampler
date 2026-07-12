Server::callWithPOST(String subURL, ComplexType parameters, Function callback) -> undefined

Thread safety: UNSAFE -- allocates a PendingCallback object and adds it to the WebThread queue. String construction and reference-counted object management are involved.
Sends an async HTTP POST request to subURL appended to the base URL. Automatically appends a trailing slash to prevent HTTP 301 redirects (unless the URL contains a dot or already ends with a slash). Parameters encoded as query params (flat JSON), JSON body (nested), or raw POST data (string). Callback receives (status, response) on the Server Thread.
Callback signature: f(int status, var response)
Required setup:
  Server.setBaseURL("https://api.example.com");
Dispatch/mechanics:
  Trailing slash appended if: no dot in subURL, no existing trailing slash, and addTrailingSlashes is true
    -> getWithParameters(subURL, parameters) builds URL
    -> PendingCallback created, queued on WebThread
    -> WebThread creates WebInputStream with POST flag, reads response, parses JSON, invokes callback
Pair with:
  setBaseURL -- must be called first to start the WebThread
  setHttpHeader -- set auth/content-type headers before calling
  setEnforceTrailingSlash -- disable automatic trailing slash if server returns 404
Anti-patterns:
  - Do NOT call before setBaseURL() -- WebThread is not running, request is silently lost
  - POST calls automatically append a trailing slash by default. This prevents 301 redirects but may cause 404 on servers that reject trailing slashes. Use setEnforceTrailingSlash(false) to disable.
  - [BUG] Passing a complex JSON object as parameters silently sets the global Content-Type header to application/json, affecting all subsequent requests until setHttpHeader() is called again
Source:
  ScriptingApi.cpp  Server::callWithPOST()
    -> trailing slash logic: !subURL.containsChar('.') && !subURL.endsWithChar('/') && globalServer.addTrailingSlashes
    -> GlobalServer::getWithParameters(subURL, parameters)
    -> GlobalServer::addPendingCallback(url, callback, true /*isPost*/)
