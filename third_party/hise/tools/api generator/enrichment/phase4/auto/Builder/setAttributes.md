Sets multiple attributes on the module at the given build index in a single call. Pass a JSON object mapping attribute names to numeric values. Only attributes that differ from the default need to be specified.

```javascript
b.setAttributes(synthIdx, {"OctaveTranspose": 5, "Detune": 0.1});
```

> [!Warning:Stops on first invalid attribute] If the JSON object contains an unrecognised attribute name, the method reports a script error and stops processing immediately. Any valid attributes listed after the invalid one are silently skipped. Place attributes you are less certain about at the end of the object to maximise the number of attributes that get applied before a potential error.
