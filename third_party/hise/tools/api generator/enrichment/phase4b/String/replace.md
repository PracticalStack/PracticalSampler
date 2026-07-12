String::replace(String search, String replacement) -> String

Thread safety: UNSAFE -- allocates new string content.
Replaces ALL occurrences of the search string with the replacement string.
Unlike JavaScript's replace() which only replaces the first match, HISEScript's
replace() replaces all occurrences.

Anti-patterns:
  - Do NOT expect JavaScript "replace first only" behavior -- HISE replace()
    replaces ALL occurrences. replace and replaceAll are identical.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::replace()
    -> a.thisObject.toString().replace(getString(a, 0), getString(a, 1))
