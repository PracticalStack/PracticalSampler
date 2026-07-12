## setStyleSheet

**Examples:**

```javascript:combined-css-scripted-laf
// Title: Combined CSS + scripted LAF (CombinedLaf pattern)
// Context: CSS handles state-driven components (buttons, combo boxes, popups)
// while scripted functions handle data-driven rendering (rotary sliders,
// filter graphs). Calling both setStyleSheet() and registerFunction() on
// the same LAF creates a CombinedLaf internally.

const var laf = Content.createLocalLookAndFeel();

// CSS handles buttons, combo boxes, and popup menus
laf.setStyleSheet("main.css");

// Scripted function handles the rotary slider (arc geometry requires
// Path.addArc with computed angles -- CSS cannot express this)
laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var ARC = 2.4;
    var bgPath = Content.createPath();
    bgPath.startNewSubPath(0.0, 0.0);
    bgPath.startNewSubPath(1.0, 1.0);
    bgPath.addArc([0, 0, 1, 1], -ARC, ARC);

    g.setColour(0xFF444444);
    g.drawPath(bgPath, obj.area, { "Thickness": 3.0 });

    var endAngle = -ARC + obj.valueNormalized * 2.0 * ARC;
    var valuePath = Content.createPath();
    valuePath.startNewSubPath(0.0, 0.0);
    valuePath.startNewSubPath(1.0, 1.0);
    valuePath.addArc([0.0, 0.0, 1.0, 1.0], -ARC, endAngle);

    g.setColour(obj.itemColour1);
    g.drawPath(valuePath, obj.area, { "Thickness": 3.0 });
});

// When HISE encounters this LAF:
// - drawRotarySlider -> script function runs (registered above)
// - drawComboBox     -> no script function, CSS renders it
// - drawToggleButton -> no script function, CSS renders it
const var knob = Content.addKnob("Knob1", 10, 10);
knob.setLocalLookAndFeel(laf);

const var combo = Content.addComboBox("Combo1", 10, 80);
combo.setLocalLookAndFeel(laf);
```

```json:testMetadata:combined-css-scripted-laf
{
  "testable": false,
  "skipReason": "Requires external CSS file (main.css) and visual rendering cannot be verified"
}
```

```javascript:separate-css-sections
// Title: Separate CSS LAF objects for different UI sections
// Context: Different sections of a plugin can use different CSS files,
// each with its own visual identity.

const var headerLaf = Content.createLocalLookAndFeel();
headerLaf.setStyleSheet("header.css");

const var keyboardLaf = Content.createLocalLookAndFeel();
keyboardLaf.setStyleSheet("keyboard.css");

// Assign to different UI sections
Content.getComponent("HeaderPanel").setLocalLookAndFeel(headerLaf);
Content.getComponent("KeyboardPanel").setLocalLookAndFeel(keyboardLaf);
```

```json:testMetadata:separate-css-sections
{
  "testable": false,
  "skipReason": "Requires external CSS files and pre-existing UI components"
}
```

**Pitfalls:**
- In the HISE IDE, if the CSS file does not exist, it is created automatically with a minimal default template (`* { color: white; }`). This means a typo in the filename silently creates a new empty file instead of producing an error. Always verify the filename matches an existing file.
