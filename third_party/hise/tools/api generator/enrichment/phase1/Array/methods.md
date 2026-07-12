# Array -- Method Entries

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clear();`

**Description:**
Removes all elements from the array without deallocating internal storage. The array becomes empty but retains its allocated capacity, so subsequent `push` calls within the previous capacity will not trigger reallocation.

**Parameters:**
None.

**Pitfalls:**
- Does not release allocated memory. To both clear and deallocate, reassign to a new empty array: `a = [];`

**Cross References:**
- `$API.Array.isEmpty$`

## clone

**Signature:** `Array clone()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new array and recursively clones all elements.
**Minimal Example:** `var b = {obj}.clone();`

**Description:**
Creates a deep copy of the array. Nested arrays and objects are recursively cloned. The returned array is fully independent -- modifying it does not affect the original. Inherited from the ObjectClass prototype, not defined on ArrayClass directly.

**Parameters:**
None.

**Pitfalls:**
- Assignment (`var b = a;`) creates a reference, not a copy. Both variables point to the same underlying array. Use `clone()` when you need an independent copy.

**Example:**
```javascript:clone-reference-vs-copy
// Title: Reference assignment vs deep copy
var a = [1, 2, 3];

var ref = a;
ref[0] = 99;
Console.print(a[0]); // 99 -- ref and a share the same array

var copy = a.clone();
copy[0] = 0;
Console.print(a[0]); // 99 -- copy is independent
```
```json:testMetadata:clone-reference-vs-copy
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["99", "99"]}
  ]
}
```

## concat

**Signature:** `undefined concat(Array arrayToConcat)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates memory for appended elements.
**Minimal Example:** `{obj}.concat([4, 5, 6]);`

**Description:**
Appends all elements from one or more arrays to this array. Modifies the array in-place and returns `undefined`. Accepts multiple array arguments via variadic parameters. Non-array arguments are silently ignored because their internal `size()` returns 0.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| arrayToConcat | Array | no | Array whose elements will be appended | Variadic; non-array values silently ignored |

**Pitfalls:**
- Unlike JavaScript's `Array.prototype.concat`, this modifies the array in-place and returns `undefined`. `var b = a.concat([4,5])` assigns `undefined` to `b`.
- Non-array arguments (numbers, strings, objects) are silently ignored -- no error, no element added. Use `push()` to append individual values.

**Cross References:**
- `$API.Array.push$`

**Example:**
```javascript:concat-in-place
// Title: In-place concatenation (differs from JavaScript)
var a = [1, 2, 3];
a.concat([4, 5], [6, 7]);
Console.print(a.length);
Console.print(a.join(", "));
```
```json:testMetadata:concat-in-place
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["7", "1, 2, 3, 4, 5, 6, 7"]}
  ]
}
```

## contains

**Signature:** `Integer contains(var value)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Linear scan O(n) over the array.
**Minimal Example:** `var found = {obj}.contains(42);`

**Description:**
Returns `true` if the array contains the specified value, `false` otherwise. Uses loose comparison (`var::operator==`), so `1` matches `1.0`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to search for | Loose comparison (1 == 1.0) |

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.includes$`
- `$API.Array.indexOf$`
- `$API.Array.clear$`

## every

**Signature:** `Integer every(Function callback)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation.
**Minimal Example:** `var allPositive = {obj}.every(function(x){ return x > 0; });`

**Description:**
Tests whether all elements pass the provided test function. Returns `true` if the callback returns a truthy value for every element, `false` if any callback returns falsy. Iteration stops at the first failing element. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Test function receiving each element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- [BUG] Returns `undefined` on an empty array instead of `true` (JavaScript returns `true` for vacuous truth). Guard with `{obj}.isEmpty() || {obj}.every(fn)` if you need correct empty-array behavior.
- Undefined/void elements are silently skipped during iteration. An array of all-undefined elements behaves as empty.

**Example:**
```javascript:every-all-even
// Title: Test if all elements satisfy a condition
var a = [2, 4, 6, 8];
Console.print(a.every(function(x){ return x % 2 == 0; }));
Console.print(a.every(function(x){ return x > 5; }));
```
```json:testMetadata:every-all-even
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1", "0"]}
  ]
}
```

**Cross References:**
- `$API.Array.some$`

## filter

**Signature:** `Array filter(Function callback)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation and a new result array.
**Minimal Example:** `var big = {obj}.filter(function(x){ return x > 10; });`

**Description:**
Creates a new array containing only the elements for which the callback returns a truthy value. The original array is not modified. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Test function; return truthy to include the element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- [BUG] Returns `undefined` on an empty array instead of an empty array. Check `{obj}.isEmpty()` before calling if you need to chain methods on the result.
- Undefined/void elements are silently skipped and never passed to the callback.

**Cross References:**
- `$API.Array.map$`

**Example:**
```javascript:filter-by-threshold
// Title: Filter elements by a threshold
var a = [1, 12, 3, 14, 5];
var big = a.filter(function(x){ return x > 10; });
Console.print(big.join(", "));
```
```json:testMetadata:filter-by-threshold
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["12, 14"]}
  ]
}
```

## find

**Signature:** `var find(Function callback)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation.
**Minimal Example:** `var item = {obj}.find(function(x){ return x > 10; });`

**Description:**
Returns the first element for which the callback returns a truthy value. Returns `undefined` if no match is found or the array is empty. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Test function; return truthy to select the element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- Undefined/void elements are silently skipped and never passed to the callback.

**Cross References:**
- `$API.Array.findIndex$`

**Example:**
```javascript:find-first-match
// Title: Find first element matching a condition
var a = [1, 12, 3, 14, 5];
var first = a.find(function(x){ return x > 10; });
Console.print(first);
```
```json:testMetadata:find-first-match
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["12"]}
  ]
}
```

## findIndex

**Signature:** `var findIndex(Function callback)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation.
**Minimal Example:** `var idx = {obj}.findIndex(function(x){ return x > 10; });`

**Description:**
Returns the index of the first element for which the callback returns a truthy value. Returns `undefined` if no match is found. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Test function; return truthy to select the element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- [BUG] Returns `undefined` when no element matches, not `-1` as in JavaScript and as `indexOf` does. Code checking `findIndex(fn) == -1` will never match. Use `isDefined(result)` to detect not-found.
- Undefined/void elements are silently skipped and never passed to the callback.

**Cross References:**
- `$API.Array.indexOf$`
- `$API.Array.find$`

**Example:**
```javascript:find-index-match
// Title: Find index of first matching element
var a = [1, 12, 3, 14, 5];
var idx = a.findIndex(function(x){ return x > 10; });
Console.print(idx);
```
```json:testMetadata:find-index-match
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1"]}
  ]
}
```

## forEach

**Signature:** `undefined forEach(Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation.
**Minimal Example:** `{obj}.forEach(function(x){ Console.print(x); });`

**Description:**
Executes the callback once for each element. Returns `undefined`. Undefined/void elements are silently skipped. For audio-thread iteration, use a `for...in` loop instead.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Function to execute for each element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- Not suitable for the audio thread. The method allocates scope objects internally. Use `for (x in array)` for allocation-free iteration.
- Undefined/void elements are silently skipped and never passed to the callback.

**Example:**
```javascript:foreach-print
// Title: Execute a function for each element
var a = ["Alice", "Bob", "Charlie"];
a.forEach(function(name){ Console.print("Hello " + name); });
```
```json:testMetadata:foreach-print
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Hello Alice", "Hello Bob", "Hello Charlie"]}
  ]
}
```

## includes

**Signature:** `Integer includes(var value)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Linear scan O(n) over the array.
**Minimal Example:** `var found = {obj}.includes(42);`

**Description:**
Alias for `contains()`. Returns `true` if the array contains the specified value, `false` otherwise. Uses loose comparison (`var::operator==`). Provided for JavaScript API compatibility.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to search for | Loose comparison (1 == 1.0) |

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.contains$`

## indexOf

**Signature:** `Integer indexOf(var elementToLookFor, int startOffset, int typeStrictness)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Linear scan O(n) over the array.
**Minimal Example:** `var idx = {obj}.indexOf("hello");`

**Description:**
Returns the index of the first occurrence of the specified value, or `-1` if not found. Searches forward from `startOffset` (default 0). The `typeStrictness` parameter controls comparison mode: 0 (default) uses loose comparison where `1 == 1.0`, while 1 uses strict comparison where both type and value must match.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| elementToLookFor | NotUndefined | no | Value to search for | -- |
| startOffset | Integer | no | Index to start searching from | Default: 0 |
| typeStrictness | Integer | no | 0 = loose comparison, 1 = strict type+value match | Default: 0 |

**Pitfalls:**
- The default loose comparison means `indexOf(1)` matches both `1` (int) and `1.0` (double). Use `typeStrictness = 1` when you need to distinguish numeric types.

**Cross References:**
- `$API.Array.lastIndexOf$`
- `$API.Array.contains$`
- `$API.Array.findIndex$`

**Example:**
```javascript:indexof-type-strictness
// Title: Loose vs strict type comparison
var a = [1, 2.0, "3"];
Console.print(a.indexOf(2));       // 1 (loose: int 2 matches double 2.0)
Console.print(a.indexOf(2, 0, 1)); // -1 (strict: int 2 != double 2.0)
```
```json:testMetadata:indexof-type-strictness
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1", "-1"]}
  ]
}
```

## insert

**Signature:** `undefined insert(int index, var value1)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Array insertion allocates and shifts elements.
**Minimal Example:** `{obj}.insert(2, "new");`

**Description:**
Inserts one or more values into the array at the specified index. Existing elements at and after the index are shifted right. Supports variadic arguments -- each additional argument is inserted sequentially starting at the given index.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based position at which to insert | -- |
| value1 | NotUndefined | no | Value to insert | Variadic: additional arguments insert at consecutive positions |

**Pitfalls:**
None.

## isArray

**Signature:** `Integer isArray(var value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `Array.isArray(someVar);`

**Description:**
Static utility that returns `true` if the argument is an array, `false` otherwise. Called on the Array prototype, not on an instance. Checks the first argument, not `this`. Equivalent to JavaScript's `Array.isArray()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to test | -- |

**Pitfalls:**
None.

## isEmpty

**Signature:** `Integer isEmpty()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isEmpty();`

**Description:**
Returns `true` if the array has no elements, `false` otherwise. HISE-specific method not available in standard JavaScript.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.clear$`

## join

**Signature:** `String join(String separator)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Always allocates strings. Guarded with WARN_IF_AUDIO_THREAD for string creation.
**Minimal Example:** `var str = {obj}.join(", ");`

**Description:**
Converts all elements to strings and concatenates them with the specified separator between each element. Returns a single string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| separator | String | no | String placed between each element | Default: empty string |

**Pitfalls:**
- Always triggers the audio thread safety warning due to string allocation. Never call from audio callbacks.

## lastIndexOf

**Signature:** `Integer lastIndexOf(var value)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Linear scan O(n) from end of array.
**Minimal Example:** `var idx = {obj}.lastIndexOf(42);`

**Description:**
Returns the index of the last occurrence of the specified value, or `-1` if not found. Searches backward from the end of the array. Always uses loose comparison (`var::operator==`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to search for | Loose comparison only |

**Pitfalls:**
- Unlike `indexOf`, there is no `typeStrictness` parameter. Comparison is always loose (`1` matches `1.0`).

**Cross References:**
- `$API.Array.indexOf$`

## map

**Signature:** `Array map(Function callback)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation and a new result array.
**Minimal Example:** `var doubled = {obj}.map(function(x){ return x * 2; });`

**Description:**
Creates a new array populated with the results of calling the provided function on every element. The original array is not modified. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Transform function; its return value becomes the new element | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- [BUG] Returns `undefined` on an empty array instead of an empty array. Check `{obj}.isEmpty()` before calling if you need to chain methods on the result.
- Undefined/void elements are silently skipped. An array `[1, undefined, 3]` produces a 2-element result, not 3.

**Cross References:**
- `$API.Array.filter$`

**Example:**
```javascript:map-transform
// Title: Transform array elements
var a = [1, 2, 3, 4];
var doubled = a.map(function(x){ return x * 2; });
Console.print(doubled.join(", "));
```
```json:testMetadata:map-transform
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["2, 4, 6, 8"]}
  ]
}
```

## pop

**Signature:** `var pop()`
**Return Type:** `NotUndefined`
**Call Scope:** safe
**Minimal Example:** `var last = {obj}.pop();`

**Description:**
Removes the last element from the array and returns it. Returns `undefined` if the array is empty.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.shift$`
- `$API.Array.push$`

## push

**Signature:** `Integer push(var value1)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Triggers audio thread warning only when the push would exceed allocated capacity. Use `reserve()` in onInit to pre-allocate.
**Minimal Example:** `{obj}.push(42);`

**Description:**
Appends one or more values to the end of the array and returns the new length. Supports variadic arguments -- each argument is appended individually.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value1 | NotUndefined | no | Value to append | Variadic: additional arguments each appended |

**Pitfalls:**
- On the audio thread, triggers a safety warning if the array needs to reallocate. Pre-allocate with `reserve()` in onInit to avoid this.

**Cross References:**
- `$API.Array.reserve$`
- `$API.Array.pushIfNotAlreadyThere$`
- `$API.Array.concat$`
- `$API.Array.pop$`

## pushIfNotAlreadyThere

**Signature:** `Integer pushIfNotAlreadyThere(var value1)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Triggers audio thread warning only when the push would exceed allocated capacity. Also performs a linear scan to check for duplicates.
**Minimal Example:** `{obj}.pushIfNotAlreadyThere(42);`

**Description:**
Appends one or more values to the end of the array only if they are not already present. Returns the new length. Uses loose comparison for duplicate checking. Supports variadic arguments -- each argument is checked independently.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value1 | NotUndefined | no | Value to append if not already present | Variadic: each argument checked independently |

**Pitfalls:**
- On the audio thread, triggers a safety warning if the array needs to reallocate. Pre-allocate with `reserve()` in onInit.
- Uses loose comparison for duplicate checking (`1` matches `1.0`).

**Cross References:**
- `$API.Array.push$`
- `$API.Array.contains$`

## remove

**Signature:** `undefined remove(var value)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Linear scan and element shifting O(n).
**Minimal Example:** `{obj}.remove(42);`

**Description:**
Removes all instances of the specified value from the array. Uses loose comparison. Does nothing if the value is not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to remove (all occurrences) | Loose comparison |

**Pitfalls:**
- Removes ALL matching instances, not just the first. To remove only at a specific index, use `removeElement()`.

**Cross References:**
- `$API.Array.removeElement$`

## removeElement

**Signature:** `undefined removeElement(int index)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Element shifting O(n) after the removed index.
**Minimal Example:** `{obj}.removeElement(0);`

**Description:**
Removes the element at the specified index. Elements after the removed index are shifted left. This is the HiseScript equivalent of JavaScript's `splice(index, 1)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index of element to remove | Out-of-range indices are silently ignored |

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.remove$`

## reserve

**Signature:** `undefined reserve(int numElements)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates memory for the specified capacity.
**Minimal Example:** `{obj}.reserve(128);`

**Description:**
Pre-allocates internal storage for at least the specified number of elements without changing the array's length or contents. Use this in `onInit` to prevent reallocation warnings when `push` is called on the audio thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numElements | Integer | no | Number of elements to allocate capacity for | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.push$`

## reverse

**Signature:** `undefined reverse()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a temporary array copy internally, allocating memory.
**Minimal Example:** `{obj}.reverse();`

**Description:**
Reverses the order of elements in the array in-place. Internally creates a reversed copy and swaps it with the original.

**Parameters:**
None.

**Pitfalls:**
- Returns `undefined`, not the array itself. Unlike `sort()` and `sortNatural()` which return the array for chaining.

## shift

**Signature:** `var shift()`
**Return Type:** `NotUndefined`
**Call Scope:** warning
**Call Scope Note:** Performance-sensitive. Shifts all remaining elements left O(n).
**Minimal Example:** `var first = {obj}.shift();`

**Description:**
Removes the first element from the array and returns it. All remaining elements are shifted left by one index. Returns `undefined` if the array is empty.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Array.pop$`

## slice

**Signature:** `Array slice(int start, int end)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new array with copied elements.
**Minimal Example:** `var sub = {obj}.slice(1, 3);`

**Description:**
Returns a shallow copy of a portion of the array from index `start` up to (but not including) index `end`. Supports negative indices: `-1` refers to the last element, `-2` to the second-to-last, etc. If `end` is omitted, extracts through the end of the array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Integer | no | Start index (inclusive). Negative values count from end. | -- |
| end | Integer | no | End index (exclusive). Negative values count from end. | Default: array length |

**Pitfalls:**
None.

**Example:**
```javascript:slice-negative-indices
// Title: Extract sub-arrays with negative indices
var a = [10, 20, 30, 40, 50];
var mid = a.slice(1, 3);
var last2 = a.slice(-2);
Console.print(mid.join(", "));
Console.print(last2.join(", "));
```
```json:testMetadata:slice-negative-indices
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["20, 30", "40, 50"]}
  ]
}
```

## some

**Signature:** `Integer some(Function callback)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates scope objects for callback invocation.
**Minimal Example:** `var hasNeg = {obj}.some(function(x){ return x < 0; });`

**Description:**
Tests whether at least one element passes the provided test function. Returns `true` if the callback returns a truthy value for any element, `false` if all elements fail. Iteration stops at the first passing element. Undefined/void elements are silently skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Test function | Must be a function |
| thisArg | NotUndefined | no | Value to use as `this` inside the callback | Optional |

**Callback Signature:** callback(element: var, index: int, array: Array)

**Pitfalls:**
- [BUG] Returns `undefined` on an empty array instead of `false` (JavaScript returns `false`). Guard with `!{obj}.isEmpty() && {obj}.some(fn)` if you need correct empty-array behavior.
- Undefined/void elements are silently skipped during iteration.

**Cross References:**
- `$API.Array.every$`

**Example:**
```javascript:some-any-match
// Title: Test if any element matches a condition
var a = [1, 2, 3, 4, 5];
Console.print(a.some(function(x){ return x > 4; }));
Console.print(a.some(function(x){ return x > 10; }));
```
```json:testMetadata:some-any-match
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1", "0"]}
  ]
}
```

## sort

**Signature:** `Array sort(Function comparator)`
**Return Type:** `Array`
**Call Scope:** warning
**Call Scope Note:** Without a comparator, sorts in-place with no allocation (safe for numeric data on audio thread). With a comparator function, allocates scope objects -- not audio-thread safe.
**Minimal Example:** `{obj}.sort();`

**Description:**
Sorts the array in-place. Without a comparator, uses the built-in VariantComparator which sorts numerically -- integers and doubles are compared by value, mixed int/double promotes to double. With a comparator function, uses stable sort (`std::stable_sort`) -- the callback receives two elements and should return a negative number if the first is less, zero if equal, positive if greater. Returns the array itself (enables chaining).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| comparator | Function | no | Comparison function. Optional -- omit for numeric sort. | Return negative if a < b, 0 if equal, positive if a > b |

**Callback Signature:** comparator(a: var, b: var)

**Pitfalls:**
- Without a comparator, only numeric arrays sort correctly. String elements all compare as equal and remain in their original order. Use `sortNatural()` for strings or provide a custom comparator.
- Without a comparator, arrays or objects as elements throw a runtime exception.
- With a comparator, uses `std::stable_sort` (preserves relative order of equal elements). Without a comparator, stability is not guaranteed.

**Cross References:**
- `$API.Array.sortNatural$`

**Example:**
```javascript:sort-numeric-and-custom
// Title: Default numeric sort and custom descending sort
var a = [3, 1, 4, 1, 5];

a.sort();
Console.print(a.join(", "));

a.sort(function(x, y){ return y - x; });
Console.print(a.join(", "));
```
```json:testMetadata:sort-numeric-and-custom
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["1, 1, 3, 4, 5", "5, 4, 3, 1, 1"]}
  ]
}
```

## sortNatural

**Signature:** `Array sortNatural()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Converts all elements to strings internally, which involves allocation.
**Minimal Example:** `{obj}.sortNatural();`

**Description:**
Sorts the array in-place using natural string comparison. All elements are converted to strings, then sorted with awareness of embedded numbers ("item2" sorts before "item10"). Returns the array itself (enables chaining). HISE-specific method not available in standard JavaScript.

**Parameters:**
None.

**Pitfalls:**
- Converts ALL elements to strings for comparison, regardless of their actual type. A mixed-type array will be sorted by string representation.

**Cross References:**
- `$API.Array.sort$`
