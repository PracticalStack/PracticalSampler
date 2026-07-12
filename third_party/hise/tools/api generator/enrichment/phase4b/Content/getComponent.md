Content::getComponent(var componentName) -> ScriptObject

Thread safety: UNSAFE -- linear search through components array (String comparison per element).
Returns a reference to the component with the given name. If not found, logs an error
and returns undefined. In the HISE IDE, also supports "throw at definition" for IDE navigation.

Dispatch/mechanics:
  Linear search through components array by name comparison
  USE_BACKEND: "throw at definition" mechanism for IDE navigation
  Not found -> logErrorAndContinue, returns var() (undefined)

Pair with:
  getAllComponents -- batch-retrieve components by regex pattern

Anti-patterns:
  - Do NOT call getComponent() inside timer callbacks, paint routines, or onControl --
    linear search on every call. Cache the reference as const var at init time.

Source:
  ScriptingApiContent.cpp:8009  Content::getComponent()
    -> linear search through components array
    -> logErrorAndContinue if not found
