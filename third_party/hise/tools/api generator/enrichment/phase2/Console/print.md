## print

**Examples:**

```javascript:tracing-a-state-machine
// Title: Tracing a state machine during development
// Context: Console.print calls left in production code serve as
// built-in documentation of control flow. They are no-ops in
// exported plugins, so there is no reason to remove them.

reg progress = 0.65;

inline function onExportStateChanged(newState)
{
    local NAMES = ["Idle", "Preparing", "Bouncing", "Writing", "Done"];
    Console.print("New export state: " + NAMES[newState]);

    if (newState == 2)
        Console.print("Bouncing... " + parseInt(progress * 100) + "%");

    if (newState == 4)
        Console.print("Export complete");
}

onExportStateChanged(2);
```
```json:testMetadata:tracing-a-state-machine
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "New export state: Bouncing",
      "Bouncing... 65%"
    ]
  }
}
```


```javascript:logging-structured-data-with
// Title: Logging structured data with trace()
// Context: For arrays and objects, Console.print shows a memory address
// unless you wrap the value in trace() to get readable JSON output.

const var filter = { "name": "Bass", "active": true };

Console.print(trace(filter));
```
```json:testMetadata:logging-structured-data-with
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "{\"name\": \"Bass\", \"active\": 1}"
    ]
  }
}
```

