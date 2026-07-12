Sets a custom callback that overrides the default product ID matching logic during key file validation. By default, version numbers are stripped so a key file for "MyPlugin 1.0" also unlocks "MyPlugin 2.0". Use this to implement version-aware or tier-specific matching. The callback receives the product ID string from the key file and must return a boolean.

```js
inline function onProductCheck(returnedId)
{
    local expectedId = Engine.getName() + " " + Engine.getVersion();
    return returnedId == expectedId;
}

ul.setProductCheckFunction(onProductCheck);
```

> [!Warning:Callback errors cause silent validation failure] If the callback throws an error or returns a non-boolean value, the product check fails silently and the key file is rejected. Ensure the callback always returns `true` or `false`.