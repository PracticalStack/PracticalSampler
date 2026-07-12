Engine::openWebsite(String url) -> undefined

Thread safety: UNSAFE -- URL object creation, 300ms deferred browser launch
Opens URL in system browser. Validates with URL::isWellFormed() -- throws script
error if malformed. Launch is deferred 300ms via DelayedFunctionCaller.
Source:
  ScriptingApi.cpp  Engine::openWebsite()
    -> URL::isWellFormed() check -> DelayedFunctionCaller(300ms) -> URL::launchInDefaultBrowser()
