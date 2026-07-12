Engine::quit() -> undefined

Thread safety: UNSAFE -- calls JUCEApplication::quit()
Signals standalone application to terminate. Complete no-op in plugin builds
(VST/AU/AAX) -- only works when IS_STANDALONE_APP is defined.
Anti-patterns:
  - No-op in plugin builds with no error or warning
Pair with:
  isPlugin -- check if running as plugin before calling
Source:
  ScriptingApi.cpp  Engine::quit()
    -> IS_STANDALONE_APP: JUCEApplication::quit()
    -> else: empty body
