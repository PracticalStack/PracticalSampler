Array::isArray(var value) -> Integer

Thread safety: SAFE
Static utility. Returns true if the argument is an array, false otherwise.
Called on the Array prototype, not on an instance: Array.isArray(someVar).

Source:
  JavascriptEngineObjects.cpp  ArrayClass::isArray()
    -> get(a, 0).isArray()
