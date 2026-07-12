## replace

**Examples:**

```javascript:chain-replace-for-labels
// Title: Chain replace calls to transform file names into display labels
// Context: Raw file names use underscores and hyphens as separators.
// A replace chain normalizes them into user-friendly display text.

var fileName = "bright_warm-pad";
var label = fileName.replace("_", " ").replace("-", " ").toUpperCase();
Console.print(label); // BRIGHT WARM PAD
```
```json:testMetadata:chain-replace-for-labels
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["BRIGHT WARM PAD"]}
  ]
}
```

```javascript:normalize-path-separators
// Title: Normalize path separators for cross-platform display
// Context: File paths may use backslashes on Windows. Replace them
// with forward slashes for consistent display and parsing.

var fullPath = "Samples\\Drums\\Kick.wav";
var normalized = fullPath.replace("\\", "/");
var parts = normalized.split("/");
Console.print(parts[2]); // Kick.wav
```
```json:testMetadata:normalize-path-separators
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Kick.wav"]}
  ]
}
```

```javascript:strip-special-characters
// Title: Strip unwanted characters from a string for identifier use
// Context: When generating a clean identifier from user-provided text,
// remove special characters that are not valid in IDs.

var userTag = "My/Tag\\Name";
var cleanTag = userTag.replace("\\", "").replace("/", "").trim();
Console.print(cleanTag); // MyTagName
```
```json:testMetadata:strip-special-characters
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["MyTagName"]}
  ]
}
```
