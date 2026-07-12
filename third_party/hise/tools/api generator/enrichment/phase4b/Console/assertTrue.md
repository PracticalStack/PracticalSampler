Console::assertTrue(NotUndefined condition) -> undefined

Thread safety: SAFE
Throws a script error if `condition` evaluates to `false`. Value is cast to `bool`, so any falsy value (0, `false`, empty string, undefined) triggers the assertion. Error message: "Assertion failure: condition is false".

Pair with: Console.assertWithMessage -- same check but with a custom error message.
