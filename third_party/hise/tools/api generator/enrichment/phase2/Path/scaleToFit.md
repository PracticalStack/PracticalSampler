## scaleToFit

**Examples:**

```javascript:scale-envelope-to-panel
// Title: Scaling an envelope path from normalized to pixel coordinates
// Context: Envelope editors build paths in normalized [0, 1] space
// (where x = time proportion, y = level) then use scaleToFit to map
// them into the actual panel dimensions. The false parameter for
// preserveProportions allows the path to stretch to fill the full
// width and height independently.

const var MARGIN = 5;
const var path = Content.createPath();

// Build envelope in normalized coordinates
path.startNewSubPath(1.0, 0.0);  // anchor top-right
path.startNewSubPath(0.0, 1.0);  // start at bottom-left

// Attack
path.quadraticTo(0.1, 0.2, 0.2, 0.0);
// Hold
path.lineTo(0.3, 0.0);
// Decay
path.quadraticTo(0.4, 0.3, 0.5, 0.4);
// Sustain
path.lineTo(0.7, 0.4);
// Release
path.quadraticTo(0.85, 0.7, 1.0, 1.0);

var boundsBeforeScale = path.getBounds(1.0);

// Scale to target dimensions with margin
const var targetWidth = 300;
const var targetHeight = 150;
path.scaleToFit(MARGIN, MARGIN, targetWidth - 2 * MARGIN,
                targetHeight - 2 * MARGIN, false);

var boundsAfterScale = path.getBounds(1.0);
```
```json:testMetadata:scale-envelope-to-panel
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "boundsAfterScale[2] > boundsBeforeScale[2]", "value": true},
    {"type": "REPL", "expression": "Math.abs(boundsAfterScale[0] - 5.0) < 1.0", "value": true}
  ]
}
```

**Pitfalls:**
- After calling `scaleToFit`, the path coordinates are permanently transformed. If you need to query positions on the path (e.g., with `getYAt`) in pixel space, call `scaleToFit` first, then query. The path cannot be "unscaled" back to its original coordinates - rebuild it if you need the original space.
