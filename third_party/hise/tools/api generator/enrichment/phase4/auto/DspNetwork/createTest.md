Creates a `ScriptNetworkTest` object for automated testing of this network. The `testData` JSON object configures the test (e.g. signal type, processing specs). The test object provides methods for running tests, setting properties, and asserting expected values.

```javascript
var test = nw.createTest({"SignalType": "Noise"});
```

> [!Warning:Backend only] Returns `undefined` silently in exported plugins. Guard any test code with a backend check to avoid calling methods on `undefined`.
