## getYAt

**Examples:**

```javascript:get-y-at-envelope-midpoint
// Title: Finding control point positions on an envelope curve
// Context: Interactive envelope editors need to know where the curve
// is at specific X positions so they can draw draggable control points.
// After building and scaling the path, getYAt samples the curve at
// the midpoint of each envelope segment to place curve-shape handles.

const var path = Content.createPath();

// Build a simple envelope in normalized space
path.startNewSubPath(0.0, 1.0);
path.quadraticTo(0.1, 0.2, 0.3, 0.0);  // attack
path.lineTo(0.5, 0.4);                   // decay to sustain
path.lineTo(0.7, 0.4);                   // sustain
path.quadraticTo(0.85, 0.7, 1.0, 1.0);  // release

// Scale to pixel space
const var MARGIN = 5;
const var panelWidth = 200;
const var panelHeight = 100;
path.scaleToFit(MARGIN, MARGIN, panelWidth - 2 * MARGIN,
                panelHeight - 2 * MARGIN, false);

// Find Y position at the midpoint of the attack segment
var midX = MARGIN + (panelWidth - 2 * MARGIN) * 0.15;

var y = path.getYAt(midX);

if (isDefined(y))
    Console.print("Y at midX: " + y);
else
    Console.print("No intersection at midX");
```
```json:testMetadata:get-y-at-envelope-midpoint
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "isDefined(y)", "value": true},
    {"type": "REPL", "expression": "y >= 5.0 && y <= 95.0", "value": true}
  ]
}
```

**Pitfalls:**
- `getYAt` operates in whatever coordinate space the path currently occupies. If the path was built in normalized [0, 1] space, call `scaleToFit` first to transform it to pixel coordinates before querying with pixel X positions. Querying a normalized path with pixel-space X values returns `undefined` because the X is outside the path's horizontal extent.
