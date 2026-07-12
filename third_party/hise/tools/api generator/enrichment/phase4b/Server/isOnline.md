Server::isOnline() -> Integer

Thread safety: UNSAFE -- blocks the scripting thread synchronously while attempting HTTP connections. Can block for up to 20 seconds (10s per URL, two URLs tried).
Checks internet connectivity by attempting to connect to Google's 204 endpoint, then Amazon as fallback. Returns true as soon as any URL responds. The HiseScript engine timeout is automatically extended by the elapsed time to prevent false script timeout errors. Does not require setBaseURL() to be called first.
Anti-patterns:
  - Do NOT call on every request -- blocks synchronously for up to 20 seconds when offline. The request callback already receives status == 0 on timeout, which handles most error cases.
  - Do NOT call in performance-sensitive contexts or tight loops. Cache the result or check only when needed (e.g., before a retry flow).
Source:
  ScriptingApi.cpp  Server::isOnline()
    -> tries URL("https://google.com/generate_204").createInputStream() with HISE_SCRIPT_SERVER_TIMEOUT
    -> falls back to URL("https://amazon.com").createInputStream()
    -> extendTimeout(elapsedMs) on the ScriptEngine to prevent false timeout errors
