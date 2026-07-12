## toArray

**Examples:**

```javascript:convert-rect-to-array
// Title: Converting a Rectangle back to [x,y,w,h] for APIs that expect arrays
// Context: Some older utility functions or third-party include files expect
// plain [x,y,w,h] arrays. Use toArray() to bridge between Rectangle objects
// and array-based APIs.

var rect = Rectangle(10, 20, 300, 200);
var header = rect.removeFromTop(50);

// Pass to a function that expects a plain array
var arr = header.toArray();

// Also useful for storing layout data as JSON-serializable values
var layoutData = {
    "header": header.toArray(),
    "content": rect.toArray()
};
```
```json:testMetadata:convert-rect-to-array
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "arr[0]", "value": 10},
    {"type": "REPL", "expression": "arr[1]", "value": 20},
    {"type": "REPL", "expression": "arr[2]", "value": 300},
    {"type": "REPL", "expression": "arr[3]", "value": 50},
    {"type": "REPL", "expression": "layoutData.content[1]", "value": 70},
    {"type": "REPL", "expression": "layoutData.content[3]", "value": 150}
  ]
}
```

Note that in most cases `toArray()` is unnecessary - Rectangle objects are accepted directly by all Graphics drawing methods and most HISE APIs. Only use it when interfacing with code that explicitly requires the `[x, y, w, h]` array format.
