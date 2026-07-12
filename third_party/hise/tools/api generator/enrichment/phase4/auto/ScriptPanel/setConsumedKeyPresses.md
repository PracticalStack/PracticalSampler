Defines which key presses this component captures when it has focus. Pass `"all"` to consume all keys exclusively, `"all_nonexclusive"` to receive all keys while still letting the parent handle them, or an array of key descriptions (strings like `"ctrl + S"` or objects with `keyCode`, `shift`, `cmd`, `alt` properties).

| Value | Behaviour |
|-------|-----------|
| `"all"` | Catches all keys exclusively - parent does not receive them |
| `"all_nonexclusive"` | Catches all keys but parent still receives them |
| Array of strings/objects | Catches only the specified key combinations |

To discover the correct key codes for your target keys:

1. Call `setConsumedKeyPresses("all")`
2. Register a callback that dumps the event: `Console.print(trace(obj))`
3. Press the desired key combinations and note the output
4. Copy the JSON objects from the console
5. Replace `"all"` with the specific key objects

> [!Warning:Call before setKeyPressCallback] Must be called before `setKeyPressCallback()`. Calling them in the wrong order reports a script error.
