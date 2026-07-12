Resets every element in the array to its default values (0 for integers, 0.0 for floats, `false` for booleans as defined by the factory prototype). The array size remains unchanged and all elements remain iterable.

> [!Warning:Does not change iteration range] Unlike `FixObjectStack.clear()`, this does not change the iteration range. All `length` elements are still visited by for-in loops after clearing - they are simply reset to defaults.
