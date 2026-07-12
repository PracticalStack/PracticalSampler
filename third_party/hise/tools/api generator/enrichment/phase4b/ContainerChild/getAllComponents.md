ContainerChild::getAllComponents(String regex) -> Array

Thread safety: UNSAFE
Recursively searches all descendant components and returns an array of
ContainerChild references whose id matches the given wildcard pattern. Returns
an empty array if no matches are found.
Anti-patterns:
  - The parameter is named "regex" but uses wildcard/glob matching (* and ?),
    NOT regular expressions. Do not pass regex syntax.
Source:
  ScriptingApiContent.cpp  ChildReference::getAllComponents()
    -> valuetree::Helpers::forEach recursive traversal
    -> RegexFunctions::matchesWildcard(regex, id) for matching
    -> parentContainer->getOrCreateChildReference() for each match
