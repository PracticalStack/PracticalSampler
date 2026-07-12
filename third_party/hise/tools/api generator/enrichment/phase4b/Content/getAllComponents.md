Content::getAllComponents(String regex) -> Array

Thread safety: UNSAFE -- iterates all components and performs regex matching (String operations).
Returns an array of component references whose names match the given regex pattern.
Pass ".*" to get all components (optimized path that skips regex matching).

Dispatch/mechanics:
  ".*" -> optimized path: returns all components without regex
  other patterns -> RegexFunctions::matchesWildcard() per component

Pair with:
  getComponent -- retrieve a single component by exact name

Anti-patterns:
  - Do NOT call inside timer callbacks or paint routines -- iterates all components
    and performs string matching. Cache results at init time.

Source:
  ScriptingApiContent.cpp:8034  Content::getAllComponents()
    -> optimized ".*" path or RegexFunctions::matchesWildcard()
    -> returns Array of ScriptComponent references
