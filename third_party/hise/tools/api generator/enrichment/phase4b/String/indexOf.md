String::indexOf(String searchString) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations. O(n) search.
Returns the zero-based index of the first occurrence of the search string, or
-1 if not found. Case-sensitive.

Pair with:
  lastIndexOf -- searches from the end instead
  contains -- boolean check without position

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::indexOf()
    -> a.thisObject.toString().indexOf(getString(a, 0))
