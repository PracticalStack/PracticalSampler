Engine::createErrorHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a ScriptErrorHandler that can react to initialization errors and compilation
failures, providing a scripting interface for customizing error reporting.
Pair with:
  showErrorMessage -- manual error display
Source:
  ScriptingApi.cpp  Engine::createErrorHandler()
    -> new ScriptErrorHandler
