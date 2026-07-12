## closeSubPath

**Examples:**

```javascript:filled-vs-open-triangle
// Title: Filled triangle vs open triangle
// Context: closeSubPath is required for shapes that need to be filled
// with Graphics.fillPath. Without it, fillPath still works but the
// path is implicitly closed by a straight line from the last point
// to the start. Explicitly closing is good practice and makes the
// intent clear.

const var filled = Content.createPath();
filled.startNewSubPath(0.0, 1.0);
filled.lineTo(0.5, 0.0);
filled.lineTo(1.0, 1.0);
filled.closeSubPath();  // explicit close for fillPath

const var open = Content.createPath();
open.startNewSubPath(0.0, 1.0);
open.lineTo(0.5, 0.0);
open.lineTo(1.0, 1.0);
// No closeSubPath - used with drawPath for an open "V" shape
```
```json:testMetadata:filled-vs-open-triangle
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "filled.getLength() > open.getLength()", "value": true},
    {"type": "REPL", "expression": "filled.contains([0.5, 0.8])", "value": true}
  ]
}
```

```javascript:closed-envelope-segment
// Title: Closed segments for per-section envelope highlighting
// Context: Envelope editors create separate closed sub-paths for
// each envelope segment (attack, decay, sustain, release) so that
// individual segments can be highlighted on hover by filling them
// with a translucent colour.

// Attack segment (filled area under the curve)
var attackPath = Content.createPath();
attackPath.startNewSubPath(0.0, 1.0);   // bottom-left
attackPath.quadraticTo(0.1, 0.2, 0.3, 0.0);  // curve to peak
attackPath.lineTo(0.3, 1.0);            // down to baseline
attackPath.closeSubPath();               // close for fill
```
```json:testMetadata:closed-envelope-segment
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "attackPath.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "attackPath.contains([0.15, 0.8])", "value": true}
  ]
}
```
