## loadFromData

**Examples:**

```javascript:load-icon-byte-array
// Title: Loading an SVG-converted icon from a byte array
// Context: The most common loadFromData pattern uses numeric arrays
// exported from SVG conversion tools. The icon is loaded once at
// initialization and drawn repeatedly in paint routines or LAF
// callbacks. The byte array is JUCE's binary path format, not raw
// SVG data.

const var iconData = [110, 109, 128, 74, 123, 67, 0, 47, 253, 67,
    108, 128, 74, 123, 67, 128, 215, 0, 68, 108, 0, 216, 125, 67,
    128, 215, 0, 68, 108, 0, 216, 125, 67, 0, 47, 253, 67, 108,
    128, 74, 123, 67, 0, 47, 253, 67, 99];

const var icon = Content.createPath();
icon.loadFromData(iconData);
```
```json:testMetadata:load-icon-byte-array
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "icon.getLength() > 0", "value": true}
}
```

```javascript:load-icons-base64
// Title: Loading icons from base64-encoded strings
// Context: Base64 encoding is more compact than byte arrays for
// storing complex path data. Many plugins maintain icon libraries
// as namespace objects with base64-encoded paths.

// Create a path, serialize it to base64, then reload from that string
const var original = Content.createPath();
original.addRectangle([0, 0, 50, 50]);
var b64 = original.toBase64();

const var reloaded = Content.createPath();
reloaded.loadFromData(b64);
```
```json:testMetadata:load-icons-base64
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "reloaded.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "original.toBase64() == reloaded.toBase64()", "value": true}
  ]
}
```

```javascript:copy-path-object
// Title: Copying path geometry from another Path object
// Context: loadFromData also accepts a Path object directly,
// which copies the geometry from the source path.

const var source = Content.createPath();
source.startNewSubPath(0.0, 0.0);
source.lineTo(1.0, 0.5);
source.lineTo(0.0, 1.0);
source.closeSubPath();

const var copy = Content.createPath();
copy.loadFromData(source);
```
```json:testMetadata:copy-path-object
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "copy.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "source.toBase64() == copy.toBase64()", "value": true}
  ]
}
```
