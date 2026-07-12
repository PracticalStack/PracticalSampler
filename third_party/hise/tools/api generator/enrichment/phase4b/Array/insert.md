Array::insert(int index, var value1) -> undefined

Thread safety: UNSAFE -- array insertion allocates and shifts elements.
Inserts one or more values at the specified index. Existing elements at and
after the index are shifted right. Supports variadic arguments -- each
additional argument is inserted sequentially.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::insert()
    -> array->insert(index++, get(a, i)) for each variadic argument
