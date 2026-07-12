Sorts the array in-place and returns the array (enables chaining). Without a comparator, sorts numerically - integers and doubles are compared by value, and mixed int/double promotes to double. With a comparator function, uses stable sort: the callback receives two elements and should return a negative number if the first is less, zero if equal, positive if greater.

For custom sorting logic beyond a simple comparator, see `Engine.sortWithFunction()`.

> [!Warning:Default sort only works on numeric arrays] Without a comparator, string elements all compare as equal and remain in their original order. Arrays or objects as elements cause a runtime exception. Use `sortNatural()` for strings or provide a custom comparator.