Sorts the array elements in place using the factory's compare function. Set a property-based or custom compare function on the factory via `FixObjectFactory.setCompareFunction()` before calling this method.

> [!Warning:Set compare function before sorting] Calling `sort()` without setting a compare function first produces meaningless ordering with no error or warning. The default comparator orders by raw memory layout, not by any property value.
