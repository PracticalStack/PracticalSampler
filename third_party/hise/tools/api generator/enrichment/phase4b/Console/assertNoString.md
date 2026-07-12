Console::assertNoString(NotUndefined value) -> undefined

Thread safety: SAFE
Throws a script error if `value` is a string. Catches accidental string-to-number coercion bugs, e.g. when a parameter that should be numeric was passed as a string.

Anti-patterns:
- The error message is "Assertion failure: " + the string value itself. This can be confusing if the string content resembles a different error message.

Pair with: Console.assertLegalNumber -- validates finite numeric value. Console.assertIsObjectOrArray -- validates object/array type.
