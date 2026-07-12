Console::assertIsObjectOrArray(NotUndefined value) -> undefined

Thread safety: SAFE
Throws a script error if `value` is not a JSON object or an array. Error message includes the actual type. Useful for validating function parameters that expect structured data.

Pair with: Console.assertLegalNumber -- validates numeric type. Console.assertNoString -- validates non-string type.
