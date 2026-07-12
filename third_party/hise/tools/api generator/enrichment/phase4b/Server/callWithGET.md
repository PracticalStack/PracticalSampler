Server::callWithGET(String subURL, ComplexType parameters, Function callback) -> undefined

Thread safety: UNSAFE -- allocates a PendingCallback object and adds it to the WebThread queue. String construction and reference-counted object management are involved.
Sends an async HTTP GET request to subURL appended to the base URL. Parameters are encoded as URL query params (flat JSON) or JSON POST body (nested objects/arrays). Callback receives (status, response) on the Server Thread; response is parsed JSON or raw string.
Callback signature: f(int status, var response)
Required setup:
  Server.setBaseURL("https://api.example.com");
Dispatch/mechanics:
  getWithParameters(subURL, parameters) builds URL with query params or JSON body
    -> PendingCallback created with WeakCallbackHolder, queued on WebThread
    -> WebThread creates WebInputStream, reads response (timeout: 10s default)
    -> response parsed as JSON (falls back to raw string), callback invoked with (status, response)
Pair with:
  setBaseURL -- must be called first to start the WebThread
  setHttpHeader -- set auth/content-type headers before calling
  setServerCallback -- monitor request queue busy/idle state
Anti-patterns:
  - Do NOT call before setBaseURL() -- WebThread is not running, request is silently lost
  - [BUG] Passing a complex JSON object (nested arrays/objects) as parameters silently sets the global Content-Type header to application/json via getWithParameters(), affecting all subsequent requests until setHttpHeader() is called again
Source:
  ScriptingApi.cpp:8047  Server::callWithGET()
    -> GlobalServer::getWithParameters(subURL, parameters) builds URL
    -> GlobalServer::addPendingCallback(url, callback, false /*isPost*/)
    -> WebThread::run() processes queue: WebInputStream -> read -> JSON::parse -> callback.call({status, response})
