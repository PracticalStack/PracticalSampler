Sets a callback that receives expansion-related error and log messages. The callback takes two parameters: a message string and a boolean indicating whether the error is critical (a critical error stops the plugin).

```javascript
eh.setErrorFunction(function(message, isCritical)
{
    if (isCritical)
        Console.print("CRITICAL: " + message);
    else
        Console.print("Warning: " + message);
});
```

Messages are triggered by expansion initialisation failures, credential validation errors, and manual calls to `setErrorMessage()`.

> [!Warning:Callback is owned by the wrapper object] The error function is owned by the ExpansionHandler wrapper, not the global expansion manager. It goes out of scope when the wrapper object is destructed. Store the wrapper in a persistent `const var` in `onInit` to keep the callback alive.
