Converts all elements to strings and concatenates them with the specified separator. A common use is building combobox item lists:

```js
const var items = ["Sine", "Triangle", "Saw"];
box.set("items", items.join("\n"));
```

To perform the inverse operation (splitting a string back into an array), use `"string".split(separator)`.

> [!Warning:Not safe on the audio thread] Always allocates strings internally. Never call from MIDI callbacks or other audio-thread code.