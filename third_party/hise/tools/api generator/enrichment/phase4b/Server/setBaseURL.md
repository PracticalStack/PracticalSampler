Server::setBaseURL(String url) -> undefined

Thread safety: UNSAFE -- constructs a URL object, stores the start time, and starts the WebThread if not already running. Thread creation involves OS-level allocation.
Sets the base URL for all subsequent GET, POST, and download requests. All sub-URLs are appended to this base. Also starts the internal WebThread -- this is the activation point for the entire server subsystem. Without calling setBaseURL(), the WebThread never starts and no requests are processed. The base URL persists on the GlobalServer, surviving script recompilations.
Anti-patterns:
  - Do NOT call callWithGET/callWithPOST/downloadFile before setBaseURL() -- the WebThread is not running and requests are silently lost. The HISE IDE emits a diagnostic warning.
Source:
  GlobalServer.cpp  GlobalServer::setBaseURL()
    -> startTime = Time::getMillisecondCounter()
    -> baseURL = URL(url)
    -> internalThread.startThread()
