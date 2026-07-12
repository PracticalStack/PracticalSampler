## lineTo

**Examples:**

```javascript:waveform-shape-icons
// Title: Building waveform shape icons
// Context: Synthesizer UIs commonly display waveform type selectors
// as small path icons. Each waveform shape is built once at init
// from startNewSubPath/lineTo sequences in normalized coordinates.

const var sinePath = Content.createPath();
const var sawPath = Content.createPath();
const var squarePath = Content.createPath();
const var trianglePath = Content.createPath();

// Sine: approximate with short line segments
sinePath.startNewSubPath(0.0, 0.0);
for (i = 0; i < 128; i += 2)
    sinePath.lineTo(i, -127 * Math.sin(2.0 * Math.PI / 126 * i));
sinePath.closeSubPath();

// Sawtooth
sawPath.startNewSubPath(0.0, 0.0);
sawPath.lineTo(0.0, 1.0);
sawPath.lineTo(1.0, -1.0);
sawPath.lineTo(1.0, 0.0);

// Square
squarePath.startNewSubPath(0.0, 0.0);
squarePath.lineTo(0.0, -1.0);
squarePath.lineTo(0.5, -1.0);
squarePath.lineTo(0.5, 1.0);
squarePath.lineTo(1.0, 1.0);
squarePath.lineTo(1.0, 0.0);
squarePath.closeSubPath();

// Triangle
trianglePath.startNewSubPath(0.0, 0.0);
trianglePath.lineTo(0.25, 1.0);
trianglePath.lineTo(0.75, -1.0);
trianglePath.lineTo(1.0, 0.0);
trianglePath.closeSubPath();
```
```json:testMetadata:waveform-shape-icons
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "sinePath.getLength() > squarePath.getLength()", "value": true},
    {"type": "REPL", "expression": "sawPath.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "squarePath.contains([0.25, -0.5])", "value": true},
    {"type": "REPL", "expression": "trianglePath.getLength() > 0", "value": true}
  ]
}
```

```javascript:filter-response-icons
// Title: Filter response curve icons for a type selector
// Context: Simple filter shape icons (HP, BP, LP) use lineTo
// to draw characteristic frequency response silhouettes.

const var hpPath = Content.createPath();
hpPath.startNewSubPath(0.0, 1.0);
hpPath.lineTo(0.25, 0.0);
hpPath.lineTo(1.0, 0.0);

const var bpPath = Content.createPath();
bpPath.startNewSubPath(0.0, 1.0);
bpPath.lineTo(0.2, 0.0);
bpPath.lineTo(0.8, 0.0);
bpPath.lineTo(1.0, 1.0);

const var lpPath = Content.createPath();
lpPath.startNewSubPath(0.0, 0.0);
lpPath.lineTo(0.75, 0.0);
lpPath.lineTo(1.0, 1.0);
```
```json:testMetadata:filter-response-icons
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "bpPath.getLength() > hpPath.getLength()", "value": true},
    {"type": "REPL", "expression": "lpPath.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "Math.abs(hpPath.getBounds(1.0)[2] - 1.0) < 0.01", "value": true}
  ]
}
```

```javascript:transport-button-icons
// Title: Transport button icons (play, stop)
// Context: Media transport controls are typically built as simple
// path shapes rendered inside toggle button LAF callbacks.

const var playIcon = Content.createPath();
playIcon.startNewSubPath(0.0, 0.0);
playIcon.lineTo(0.0, 1.0);
playIcon.lineTo(1.0, 0.5);
playIcon.closeSubPath();

const var stopIcon = Content.createPath();
stopIcon.startNewSubPath(0.0, 0.0);
stopIcon.lineTo(0.0, 1.0);
stopIcon.lineTo(1.0, 1.0);
stopIcon.lineTo(1.0, 0.0);
stopIcon.closeSubPath();
```
```json:testMetadata:transport-button-icons
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "playIcon.contains([0.3, 0.5])", "value": true},
    {"type": "REPL", "expression": "stopIcon.contains([0.5, 0.5])", "value": true}
  ]
}
```

```javascript:combo-box-frame-path
// Title: Custom combo box frame with dropdown arrow
// Context: A non-rectangular combo box shape defined as a single
// path with an integrated arrow indicator.

const var comboFrame = Content.createPath();
comboFrame.startNewSubPath(0.0, 0.0);
comboFrame.lineTo(1.0, 0.0);
comboFrame.lineTo(1.0, 0.66);
comboFrame.lineTo(0.69, 0.66);
comboFrame.lineTo(0.6, 1.0);
comboFrame.lineTo(0.0, 1.0);
comboFrame.closeSubPath();

// Add a small dropdown triangle as a second sub-path
comboFrame.startNewSubPath(0.8, 0.2);
comboFrame.lineTo(0.9, 0.2);
comboFrame.lineTo(0.85, 0.45);
comboFrame.closeSubPath();
```
```json:testMetadata:combo-box-frame-path
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "comboFrame.contains([0.5, 0.5])", "value": true},
    {"type": "REPL", "expression": "comboFrame.contains([0.85, 0.3])", "value": true}
  ]
}
```
