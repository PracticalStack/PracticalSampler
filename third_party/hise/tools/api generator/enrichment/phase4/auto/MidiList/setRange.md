Fills a contiguous range of slots with the same value. The second parameter (`numToFill`) acts as an absolute end index, not a count - `setRange(0, 12, 100)` fills slots 0 through 11.

> [!Warning:Second parameter is end bound, not count] Because `numToFill` is an absolute end bound (not a count relative to `startIndex`), calling `setRange(10, 5, 99)` fills zero slots since the loop condition is immediately false. Always ensure `numToFill` is greater than `startIndex`.
