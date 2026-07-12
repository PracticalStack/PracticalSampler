## createMarkdownRenderer

**Examples:**

```javascript:styled-markdown-tooltip
// Title: Rich tooltip system with styled markdown
// Context: A tooltip panel that renders formatted text with custom styling.
// The full workflow: create renderer -> style it -> set bounds -> draw.

Content.makeFrontInterface(900, 600);

const var tooltipPanel = Content.addPanel("TooltipPanel", 0, 0);
tooltipPanel.set("width", 300);
tooltipPanel.set("height", 200);

const var md = Content.createMarkdownRenderer();

// Customize the style data
const var style = md.getStyleData();
style.headlineColour = 0xFF333333;
style.textColour = 0xFF444444;
style.Font = "regular";
style.FontSize = 13.0;
style.UseSpecialBoldFont = false;
style.tableLineColour = 0x11FFFFFF;
style.tableHeaderBgColour = 0;
style.tableBgColour = 0;
md.setStyleData(style);

// Set the markdown content
md.setText("### Controls\nAdjust the **gain** and **mix** parameters.");

tooltipPanel.setPaintRoutine(function(g)
{
    g.setColour(0xEE222222);
    g.fillRoundedRectangle(this.getLocalBounds(0), 6.0);

    // setTextBounds must be called before drawMarkdownText
    // It returns the actual height needed for the text
    var bounds = [10, 10, this.get("width") - 20, 2000];
    var textHeight = md.setTextBounds(bounds);

    g.drawMarkdownText(md);
});
```
```json:testMetadata:styled-markdown-tooltip
{
  "testable": false,
  "skipReason": "Paint routine rendering is visual-only and cannot be verified via REPL"
}
```
