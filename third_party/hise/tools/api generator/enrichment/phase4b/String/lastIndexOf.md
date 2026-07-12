String::lastIndexOf(String searchString) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations. O(n) search.
Returns the zero-based index of the last occurrence of the search string, or
-1 if not found. Case-sensitive.

Pair with:
  indexOf -- searches from the beginning instead

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::lastIndexOf()
    -> a.thisObject.toString().lastIndexOf(getString(a, 0))
