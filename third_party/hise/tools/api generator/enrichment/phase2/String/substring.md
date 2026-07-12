## substring

**Examples:**

```javascript:strip-known-prefix
// Title: Strip a known prefix from a module attribute name
// Context: Module attributes often have a common prefix (e.g., "Master"
// or "SendFx"). Use substring to remove the prefix before further
// processing like splitCamelCase.

var attribute = "MasterCompThreshold";

// Skip the "Master" prefix (6 characters)
var paramName = attribute.substring(6, 10000).splitCamelCase().join(" ");
Console.print(paramName); // Comp Threshold
```
```json:testMetadata:strip-known-prefix
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Comp Threshold"]}
  ]
}
```

```javascript:extract-fixed-format-index
// Title: Extract a channel index from a fixed-format component ID
// Context: When component IDs follow a strict naming format like
// "MixerStrip01", substring extracts a specific character range
// to parse as a number.

var componentId = "MixerStrip07";

// Extract characters 10-12 to get the two-digit index
var idx = parseInt(componentId.substring(10, 12));
Console.print(idx); // 7
```
```json:testMetadata:extract-fixed-format-index
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["7"]}
  ]
}
```

```javascript:prefix-type-dispatch
// Title: Check a string prefix for type dispatch
// Context: When component IDs encode their type in the first few
// characters, substring provides a lightweight prefix check.

var id = "FilterCutoff1";

if (id.substring(0, 6) == "Filter")
    Console.print("Filter parameter");
else if (id.substring(0, 6) == "Player")
    Console.print("Player parameter");
```
```json:testMetadata:prefix-type-dispatch
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Filter parameter"]}
  ]
}
```

**Pitfalls:**
- Always pass two arguments. Unlike JavaScript, omitting the end index is unreliable. Use a large number like 10000 to mean "rest of string" - HISE clamps it to the actual string length.
