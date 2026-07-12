Array::reserve(int numElements) -> undefined

Thread safety: UNSAFE -- allocates memory for the specified capacity.
Pre-allocates internal storage without changing the array's length or contents.
Call in onInit to prevent reallocation warnings when push is called on the
audio thread.

Required setup:
  // In onInit:
  var noteIds = [];
  noteIds.reserve(128);

Pair with:
  push -- append elements within pre-allocated capacity
  pushIfNotAlreadyThere -- append unique elements within pre-allocated capacity

Source:
  JavascriptEngineObjects.cpp  ArrayClass::reserve()
    -> array->ensureStorageAllocated(numElements)
