## setText

**Examples:**

```javascript:modal-dialog-text
// Title: Update dialog content dynamically
// Context: A modal dialog uses markdown for its title and body text,
// updating both when the dialog is shown with different messages.

const var dialogPanel = Content.addPanel("DialogPanel", 0, 0);
dialogPanel.set("width", 450);
dialogPanel.set("height", 250);
dialogPanel.set("visible", false);

const var md = Content.createMarkdownRenderer();

// Set initial placeholder text
md.setText("#### Save Preset\nEnter the name of the preset.");

inline function showDialog(title, body)
{
    // Build markdown string with heading + body each time
    md.setText("#### " + title + "\n" + body);
    dialogPanel.set("visible", true);
    dialogPanel.repaint();
}

dialogPanel.setPaintRoutine(function(g)
{
    g.setColour(0xFFDDDDDD);
    g.fillRoundedRectangle(this.getLocalBounds(0), 10);

    local box = [40, 20, this.getWidth() - 80, this.getHeight() - 40];
    md.setTextBounds(box);
    g.drawMarkdownText(md);
});

// Show different dialogs by changing the text
showDialog("Confirm Delete", "Are you sure you want to **delete** this item?");
```

```json:testMetadata:modal-dialog-text
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "dialogPanel.get('visible')", "value": true},
    {"type": "REPL", "expression": "md.setTextBounds([0, 0, 370, 1000]) > 0", "value": true}
  ]
}
```

```javascript:broadcaster-driven-updates
// Title: Update markdown reactively via broadcaster
// Context: An error display panel listens to a status broadcaster
// and updates its markdown text when the status changes.

const var statusPanel = Content.addPanel("StatusPanel", 0, 0);
statusPanel.set("width", 400);
statusPanel.set("height", 150);

const var errorMd = Content.createMarkdownRenderer();
errorMd.setTextBounds([20, 20, 360, 120]);

const var statusBroadcaster = Engine.createBroadcaster({
    "id": "statusBc",
    "args": ["state", "message"]
});

statusBroadcaster.addListener(statusPanel, "update error text",
    function(state, message)
    {
        if (state != 0)
        {
            // setText() parses the markdown immediately
            this.data.errorMd.setText(message);
            this.repaint();
        }
    }
);

statusPanel.data.errorMd = errorMd;

statusPanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.drawMarkdownText(this.data.errorMd);
});
```

```json:testMetadata:broadcaster-driven-updates
{
  "testable": false,
  "skipReason": "Listener effect (setText on renderer) has no scriptable readback; verification requires visual rendering"
}
```
