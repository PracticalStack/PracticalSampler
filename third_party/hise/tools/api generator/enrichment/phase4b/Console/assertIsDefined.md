Console::assertIsDefined(NotUndefined value) -> undefined

Thread safety: SAFE
Throws a script error if `value` is `undefined` or void. Useful for validating that a variable has been initialised or that a function returned a meaningful result.

Pair with: Console.assertTrue -- for general boolean condition checks.
