## assertTrue

**Examples:**

```javascript:guard-clauses-in-a
// Title: Guard clauses in a data-binding utility
// Context: Validate preconditions at the start of utility functions
// that access panel data properties. Each assertion targets a specific
// failure mode that would otherwise produce a cryptic error downstream.

const var panel = Content.addPanel("TestPanel", 0, 0);
panel.data.testProperty = 42;

inline function setProperty(panel, name, value)
{
    Console.assertTrue(isDefined(panel));
    Console.assertTrue(isDefined(name));
    Console.assertTrue(isDefined(panel.data));
    Console.assertTrue(isDefined(panel.data[name]));

    panel.data[name] = value;
}

setProperty(panel, "testProperty", 100);
```
```json:testMetadata:guard-clauses-in-a
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "panel.data.testProperty",
    "value": 100
  }
}
```


```javascript:unreachable-code-marker
// Title: Unreachable code marker
const DATA_LIST = [{Key: 'A', Name: 'Alpha'}, {Key: 'B', Name: 'Beta'}];

inline function findNameByKey(key)
{
    for (entry in DATA_LIST)
        if (entry.Key == key)
            return entry.Name;

    // If we get here, the key was not in DATA_LIST - a programming error
    Console.assertTrue(false);
    return "";
}

// Invoke with invalid key to trigger assertion
const result = findNameByKey('Z');
```
```json:testMetadata:unreachable-code-marker
{
  "testable": true,
  "verifyScript": {
    "type": "expect-error",
    "errorMessage": "Assertion failure: condition is false"
  }
}
```


```javascript:validating-a-whitelist-of
// Title: Validating a whitelist of allowed keys
// Context: When a function accepts a string key that must be one of
// a known set, assertTrue with indexOf catches typos at the call site.

const var settings = {"AutoPlay": true, "Volume": 0.8, "Quality": "High"};

inline function changeSetting(key, value)
{
    Console.assertTrue(["AutoPlay", "Volume", "Quality"].indexOf(key) != -1);
    settings[key] = value;
}

changeSetting("Volume", 0.5);
```
```json:testMetadata:validating-a-whitelist-of
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "settings.Volume",
    "value": 0.5
  }
}
```

