## setTextBounds

**Examples:**

```javascript:auto-size-tooltip
// Title: Auto-size a tooltip panel to fit markdown content
// Context: A tooltip system measures how tall the rendered markdown
// will be and resizes the host panel accordingly.

const var MARGIN = 14;
const var SHADOW = 10;

const var tooltipMd = Content.createMarkdownRenderer();

const var tooltipPanel = Content.addPanel("TooltipPanel", 0, 0);
tooltipPanel.set("width", 300);
tooltipPanel.set("visible", false);

inline function showTooltip(panel, text)
{
    if (text.length == 0)
    {
        panel.set("visible", false);
        return;
    }

    panel.set("visible", true);
    tooltipMd.setText(text);

    // Measure content height at the available width
    local contentWidth = panel.getWidth() - 2 * MARGIN - 2 * SHADOW;
    local textBounds = [MARGIN + SHADOW, MARGIN + SHADOW, contentWidth, 2000];
    local contentHeight = tooltipMd.setTextBounds(textBounds);

    // Resize panel to fit the content plus margins
    local totalHeight = contentHeight + 2 * MARGIN + 2 * SHADOW;
    panel.set("height", totalHeight);
    panel.repaint();
}

tooltipPanel.setPaintRoutine(function(g)
{
    g.setColour(0xFFDDDDDD);
    g.fillRoundedRectangle(this.getLocalBounds(SHADOW), 6);
    g.drawMarkdownText(tooltipMd);
});

showTooltip(tooltipPanel, "### Controls\nUse the **knob** to adjust the value.\n- Turn right to increase\n- Turn left to decrease");
```

```json:testMetadata:auto-size-tooltip
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tooltipPanel.get('visible')", "value": true},
    {"type": "REPL", "expression": "tooltipPanel.get('height') > 48", "value": true}
  ]
}
```
