Server::setTimeoutMessageString(String timeoutMessage) -> undefined

Thread safety: SAFE -- wraps the string in a var and assigns it to an atomic-safe field on the WebThread.
Sets the string used as the response body when a server request times out (default timeout: 10s via HISE_SCRIPT_SERVER_TIMEOUT). Default value is "{}" (empty JSON object string). The timeout message is passed to the callback's response argument alongside status code Server.StatusNoConnection (0). Changing this to a custom string allows distinguishing between a timeout and a successful empty response. Persists across script recompilations.
Pair with:
  callWithGET / callWithPOST -- the methods whose timeout responses use this string
Source:
  ScriptingApi.cpp  Server::setTimeoutMessageString()
    -> sets WebThread::timeoutMessage var field
    -> WebThread::run() uses timeoutMessage when WebInputStream is null (timeout case)
