## addNoise

**Examples:**

```javascript:addnoise-panel-texture
// Title: Subtle background noise texture on a panel
// Context: A thin monochromatic noise layer adds visual texture to flat
// backgrounds, making them feel less sterile. This is typically the last
// drawing operation in a panel's paint routine, applied on top of the
// filled background.

const var bgPanel = Content.addPanel("bgPanel", 0, 0);
bgPanel.set("width", 400);
bgPanel.set("height", 300);

bgPanel.setPaintRoutine(function(g)
{
    g.setColour(0xFF3A3B3E);
    g.fillRoundedRectangle(this.getLocalBounds(0), 4.0);

    // Noise is the last step -- draws on top of everything
    g.addNoise({"alpha": 0.02, "monochromatic": true});
});
```
```json:testMetadata:addnoise-panel-texture
{
  "testable": false,
  "skipReason": "Visual output only -- noise overlay cannot be verified programmatically"
}
```

**Pitfalls:**
- When using the simple float form (`g.addNoise(0.05)`) inside a LAF callback, the parent component may not be a ScriptPanel, causing the area to default to an empty rectangle. Use the JSON object form with an explicit `area` property in LAF callbacks: `g.addNoise({"alpha": 0.05, "monochromatic": true, "area": obj.area})`.
- A noise alpha of 0.02-0.05 is typical for subtle texture. Values above 0.1 produce a visibly grainy surface that can look like a rendering bug.
