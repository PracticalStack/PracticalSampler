Returns the section of the string from start to end (exclusive). `String.slice()` is an identical alias.

> [!Warning:Always pass two arguments] Unlike JavaScript, omitting the end index is unreliable. Pass a large number like `10000` to mean "rest of string" - HISE clamps it to the actual string length.
