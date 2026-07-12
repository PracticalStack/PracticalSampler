## capitalize

**Examples:**

```javascript:format-raw-filename
// Title: Format raw file names into title-case display labels
// Context: Audio file names often use underscores and lowercase.
// Replace separators with spaces, then capitalize for display.

var rawName = "bright_warm_pad";
var displayName = rawName.replace("_", " ").capitalize();
Console.print(displayName); // Bright Warm Pad
```
```json:testMetadata:format-raw-filename
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Bright Warm Pad"]}
  ]
}
```

```javascript:display-name-from-path
// Title: Build a display name from the tail of a file path
// Context: After stripping a common prefix with substring, capitalize
// the remaining portion for use as a user-facing label.

var fullName = "factory-preset-warm-keys";
var shortName = fullName.substring(15, 10000);

// "warm-keys" -> "warm keys" -> "Warm Keys"
var label = shortName.replace("-", " ").capitalize();
Console.print(label); // Warm Keys
```
```json:testMetadata:display-name-from-path
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Warm Keys"]}
  ]
}
```
