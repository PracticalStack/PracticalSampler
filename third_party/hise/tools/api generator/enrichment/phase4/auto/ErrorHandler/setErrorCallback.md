Registers a callback that fires when an error state changes. The callback receives two arguments: the state constant of the highest-priority active error and the resolved error message string. Replaces any previously registered callback.

```
eh.setErrorCallback(function(state, message)
{
    // state:   ErrorHandler constant (0-13)
    // message: resolved error string
});
```