## removeFromLeft

**Examples:**

```javascript:dynamic-tag-bar
// Title: Dynamic tag bar with variable-width labels
// Context: Building a row of tags where each tag's width depends on its text content.
// Progressive removeFromLeft calls allocate exactly the space each tag needs.

const var tagPanel = Content.addPanel("TagPanel", 0, 0);
tagPanel.set("width", 400);
tagPanel.set("height", 30);

const var TAGS = ["Drums", "Bass", "Synth Lead", "FX"];

tagPanel.data.tagAreas = [];

// Pre-compute tag areas during initialization
var b = Rectangle(tagPanel.getLocalBounds(0));
b.removeFromLeft(8); // left margin

for (tag in TAGS)
{
    var w = Engine.getStringWidth(tag.toUpperCase(), "Oxygen Bold", 11, 0.03) + 18;
    tagPanel.data.tagAreas.push(b.removeFromLeft(w));
}

tagPanel.setPaintRoutine(function(g)
{
    for (i = 0; i < TAGS.length; i++)
    {
        g.setColour(0xFF444444);
        g.fillRoundedRectangle(this.data.tagAreas[i].reduced(2, 8), 2);
        g.setColour(0xFFCCCCCC);
        g.setFont("Oxygen Bold", 11);
        g.drawAlignedText(TAGS[i].toUpperCase(), this.data.tagAreas[i], "centred");
    }
});
```
```json:testMetadata:dynamic-tag-bar
{
  "testable": false,
  "skipReason": "Paint routine requires panel rendering, cannot be tested standalone"
}
```

```javascript:envelope-proportional-segments
// Title: Envelope segment layout with proportional widths
// Context: Dividing an area into proportional time segments for a custom
// AHDSR envelope editor. Each removeFromLeft call claims a fraction of the
// total width based on the segment's time proportion.

const var envPanel = Content.addPanel("EnvPanel", 0, 0);
envPanel.set("width", 300);
envPanel.set("height", 100);

inline function updateEnvelopeLayout(panel, data)
{
    var area = Rectangle(panel.getLocalBounds(10));
    var w = area.width;

    local segments = [];

    // Each segment claims a proportional strip from the left
    segments.push(area.removeFromLeft(w * data.attack));
    segments.push(area.removeFromLeft(w * data.hold));
    segments.push(area.removeFromLeft(w * data.decay));
    segments.push(area.removeFromLeft(w * data.sustain));
    segments.push(area.removeFromLeft(w * data.release));

    panel.data.segments = segments;
    panel.repaint();
}
```
```json:testMetadata:envelope-proportional-segments
{
  "testable": false,
  "skipReason": "Incomplete example - utility function requires caller with specific data object and panel rendering"
}
```

**Pitfalls:**
- When slicing stored areas (like `obj.area` from a LAF callback), wrap in `Rectangle()` first to avoid mutating the original. If `obj.area` is still an array, calling a Rect namespace `removeFromLeft` on it mutates the array in-place, which corrupts the area for subsequent repaints.
