## assertIsDefined

**Examples:**

```javascript:guarding-builder-api-module
// Title: Guarding Builder API module creation
// Context: After creating objects or retrieving properties, assertIsDefined
// catches undefined values immediately rather than letting them propagate.

const var config = {
    "oscillator": {"type": "sine", "gain": 0.5},
    "filter": {"cutoff": 1000, "resonance": 0.7}
};

inline function validateConfig(cfg)
{
    Console.assertIsDefined(cfg.oscillator);
    Console.assertIsDefined(cfg.filter);
    Console.assertIsDefined(cfg.oscillator.type);
    Console.assertIsDefined(cfg.filter.cutoff);
}

validateConfig(config);
```
```json:testMetadata:guarding-builder-api-module
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "config.oscillator.gain",
    "value": 0.5
  }
}
```


```javascript:validating-object-properties-before
// Title: Validating object properties before use
// Context: When reading structured data from JSON or configuration objects,
// assertIsDefined confirms that expected properties exist.

inline function loadActivationConfig()
{
    local keyFile = FileSystem.getFolder(FileSystem.AudioFiles)
                    .getParentDirectory()
                    .getChildFile("RSA.xml");

    local obj = keyFile.loadFromXmlFile();

    Console.assertIsDefined(obj.PublicKey);
    Console.assertIsDefined(obj.PrivateKey);
}

```
```json:testMetadata:validating-object-properties-before
{
  "testable": false
}
```


```javascript:validating-callback-state
// Title: Validating callback state
// Context: Before invoking a stored callback reference, confirm it was
// actually assigned. A missing callback is a programming error, not
// a valid state.

var currentOKCallback;

inline function showConfirmDialog(message, okCallback)
{
    currentOKCallback = okCallback;
    // ... show dialog
}

inline function onDialogOK()
{
    Console.assertIsDefined(currentOKCallback);
    currentOKCallback();
}

// Set up a callback and invoke it
showConfirmDialog("Test", function() {
    Console.print("Callback executed");
});

onDialogOK();
```
```json:testMetadata:validating-callback-state
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "Callback executed"
    ]
  }
}
```

