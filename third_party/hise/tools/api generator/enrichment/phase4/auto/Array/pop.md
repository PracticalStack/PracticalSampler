Removes and returns the last element. Returns `undefined` if the array is empty. When working with arrays that may be empty or contain sparse indices, guard the result with `isDefined()`:

```js
var item = a.pop();
if (isDefined(item))
    Console.print(item);
```

Safe on the audio thread when used within pre-allocated capacity (paired with `push` and `reserve`).