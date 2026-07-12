Server::setEnforceTrailingSlash(Integer shouldAddSlash) -> undefined

Thread safety: SAFE -- sets a plain boolean field on the GlobalServer. No allocations, no locks.
Controls whether POST requests automatically append a trailing slash to the sub-URL. Default is true. When enabled, callWithPOST() appends a "/" to prevent HTTP 301 redirects that convert POST to GET. The slash is NOT added if the sub-URL contains a dot (file endpoint) or already ends with a slash. Set to false if the target server returns 404 on trailing slashes. Only affects POST calls -- GET calls are not modified. Persists across script recompilations.
Pair with:
  callWithPOST -- the only method affected by this setting
Source:
  ScriptingApi.cpp  Server::setEnforceTrailingSlash()
    -> sets GlobalServer::addTrailingSlashes boolean field
