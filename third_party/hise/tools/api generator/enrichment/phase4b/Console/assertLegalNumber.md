Console::assertLegalNumber(NotUndefined value) -> undefined

Thread safety: SAFE
Throws a script error if `value` is not a finite, legal number. Performs two checks: verifies the value is numeric (not string/object/array), then checks it is not `NaN` or infinity via HISE's `FloatSanitizers`. Error message includes the type and/or value on failure.

Pair with: Console.assertNoString -- validates non-string type. Console.assertIsObjectOrArray -- validates object/array type.
