## setLocalLookAndFeel

**Examples:**

```javascript:shared-table-laf
// Title: Style multiple table editors with one local look-and-feel
// Context: A modulation page applies one table-specific LAF to all curve editors.

const var tableA = Content.addTable("TableA", 10, 10);
const var tableB = Content.addTable("TableB", 10, 130);

const var tableLaf = Content.createLocalLookAndFeel();

tableLaf.registerFunction("drawTableBackground", function(g, obj)
{
    g.setColour(0xFF202225);
    g.fillRoundedRectangle(obj.area, 4.0);
});

tableLaf.registerFunction("drawTablePath", function(g, obj)
{
    g.setGradientFill([obj.itemColour, 0.0, 0.0, obj.itemColour2, 0.0, obj.area[3], false]);
    g.drawPath(obj.path, Rectangle(obj.area), 1.5);
});

tableLaf.registerFunction("drawTablePoint", function(g, obj)
{
    g.setColour(obj.hover ? Colours.white : 0xAAFFFFFF);
    g.fillEllipse(Rectangle(obj.tablePoint).reduced(2));
});

tableLaf.registerFunction("drawTableRuler", function(g, obj)
{
    g.setColour(0x88FFFFFF);
    g.fillRect([obj.position * obj.area[2], 0, 2, obj.area[3]]);
});

tableA.setLocalLookAndFeel(tableLaf);
tableB.setLocalLookAndFeel(tableLaf);
```
```json:testMetadata:shared-table-laf
{
  "testable": false,
  "skipReason": "LookAndFeel draw callbacks are render-path behavior and cannot be asserted through script state"
}
```

**Cross References:**
- `ScriptLookAndFeel.registerFunction`
