Sets the comparison function used by all arrays and stacks from this factory for sorting, `indexOf()`, and `contains()` operations. Accepts three input modes:

1. **Single property name** (e.g. `"score"`): an optimised C++ comparator that reads the property's memory directly with no script callback overhead.
2. **Multi-property string** (e.g. `"category,score"`): compares 2-4 properties in priority order - the first property that differs determines the result.
3. **Custom function**: a callback receiving two objects that returns `-1`, `0`, or `1`. Provides full flexibility but incurs script callback overhead on every comparison.

Passing any other value (e.g. `0`) resets to the default comparator, which uses byte-level equality for matching.

> [!Warning:Comparator propagates to all containers] The comparator propagates retroactively to all arrays and stacks previously created from this factory. There is no way to set a per-container comparator independently.

> [!Warning:Custom JS comparator is slower] A custom JavaScript comparison function makes sorting and search operations significantly slower due to synchronous script callback overhead. Prefer the string-based property comparator for performance-critical code.
