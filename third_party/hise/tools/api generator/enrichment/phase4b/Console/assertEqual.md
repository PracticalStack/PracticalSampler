Console::assertEqual(NotUndefined v1, NotUndefined v2) -> undefined

Thread safety: SAFE
Throws a script error if `v1` and `v2` are not equal. Uses `!=` operator -- compares by value for primitives, by reference for objects. Error message includes string representation of both values.

Pair with: Console.assertTrue -- for boolean condition checks. Console.assertWithMessage -- for custom error messages.
