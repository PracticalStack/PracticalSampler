Console::assertWithMessage(Integer condition, String errorMessage) -> undefined

Thread safety: SAFE
Throws a script error with the provided `errorMessage` if `condition` is false. Most flexible assertion -- lets you specify exactly what went wrong. Error is prefixed with "Assertion failure: ".

Pair with: Console.assertTrue -- simpler assertion without custom message.
