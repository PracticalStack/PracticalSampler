## setContentData

**Examples:**

```javascript
// Title: Embed a preset browser with custom layout and styling
// Context: The most common floating tile use case -- embedding a fully
// configured preset browser with two-column layout and custom LAF

const var presetTile = Content.addFloatingTile("PresetTile", 0, 0);
presetTile.setPosition(10, 10, 600, 300);

presetTile.setContentData({
    "Type": "PresetBrowser",
    "ShowSaveButton": false,
    "ShowExpansionsAsColumn": false,
    "ShowFolderButton": false,
    "ShowNotes": false,
    "ShowEditButtons": false,
    "ShowFavoriteIcon": false,
    "NumColumns": 2,
    "ColumnWidthRatio": [0.3, 0.7],
    "ListAreaOffset": [0, 0, 0, 0],
    "SearchBarBounds": [0, 0, 0, 0]
});

// Set colours after setContentData -- a content reload may reset them
presetTile.set("bgColour", 0);
presetTile.set("FontSize", 14);

// Attach custom look and feel for list item rendering
const var browserLaf = Content.createLocalLookAndFeel();

browserLaf.registerFunction("drawPresetBrowserColumnBackground", function(g, obj)
{
    g.setColour(0xDD1A1A1A);
    g.fillRoundedRectangle(obj.area, 5);
});

browserLaf.registerFunction("drawPresetBrowserListItem", function(g, obj)
{
    var c = obj.selected ? 0xFFC9EAF1
                         : Colours.withAlpha(Colours.white, obj.hover ? 0.7 : 0.4);

    if (obj.selected)
    {
        g.setColour(0x09FFFFFF);
        g.fillRoundedRectangle(obj.area, 5);
    }

    g.setFont("Default", 14.0);
    g.setColour(c);
    obj.area[0] += 10;
    g.drawAlignedText(obj.text, obj.area, "left");
});

presetTile.setLocalLookAndFeel(browserLaf);
```

```javascript
// Title: Interactive EQ display with broadcaster-driven processor switching
// Context: An EQ panel that switches between different processor instances
// when the user selects a different EQ band (e.g., master/mid/side)

const var eqData = {
    "Type": "DraggableFilterPanel",
    "ProcessorId": "MasterEQ",
    "Index": 0,
    "AllowFilterResizing": false,
    "AllowDynamicSpectrumAnalyser": 1,
    "UseUndoManager": true,
    "GainRange": 12.0,
    "AllowContextMenu": false,
    "ResetOnDoubleClick": true
};

const var eqTile = Content.addFloatingTile("EQPanel", 0, 0);
eqTile.setPosition(10, 10, 400, 250);
eqTile.setContentData(eqData);

const var eqLaf = Content.createLocalLookAndFeel();
eqTile.setLocalLookAndFeel(eqLaf);

// Switch the EQ target when a radio group changes.
// IMPORTANT: clone the data before modifying -- mutating the
// original template affects all future calls.
const var EQ_NAMES = ["MasterEQ", "MidEQ", "SideEQ"];

const var eqSelector = Engine.createBroadcaster({
    "id": "eqSelector",
    "args": ["index"]
});

eqSelector.addListener(eqTile, "retarget EQ panel", function(index)
{
    var newData = eqData.clone();
    newData.ProcessorId = EQ_NAMES[index];
    this.setContentData(newData);
});
```

```javascript
// Title: Reusable peak meter factory with dynamic channel targeting
// Context: A factory function creates identically configured peak meter
// tiles that can be retargeted to different processors at runtime

namespace PeakMeter
{

const var BASE_DATA = {
    "Type": "MatrixPeakMeter",
    "ProcessorId": "",
    "ChannelIndexes": [0, 1],
    "DownDecayTime": 1400,
    "SkewFactor": 0.5,
    "UseSourceChannels": true,
    "ShowMaxPeak": true,
    "PaddingSize": 2.0,
    "SegmentLedSize": 0.0
};

inline function make(name, processorId, isVertical)
{
    local ft = Content.addFloatingTile(name, 0, 0);

    local data = BASE_DATA.clone();
    data.ProcessorId = processorId;
    ft.setContentData(data);

    if (isVertical)
    {
        ft.set("width", 10);
        ft.set("height", 108);
    }
    else
    {
        ft.set("height", 10);
        ft.set("width", 108);
    }

    return ft;
}

// Retarget an existing meter to a different processor
inline function setTarget(floatingTile, processorId)
{
    local data = BASE_DATA.clone();
    data.ProcessorId = processorId;
    floatingTile.setContentData(data);
}

} // namespace PeakMeter

// Usage
const var masterMeter = PeakMeter.make("MasterMeter", "MainSynth", true);
masterMeter.setPosition(780, 20, 10, 108);
```

```javascript
// Title: Filter display connected to a specific oscillator processor
// Context: Per-oscillator filter displays embedded as children of
// their parent oscillator panel

const var NUM_OSCILLATORS = 2;

for (i = 0; i < NUM_OSCILLATORS; i++)
{
    local fd = Content.addFloatingTile("FilterDisplay" + (i + 1), 100, 50);
    fd.set("width", 150);
    fd.set("height", 45);
    fd.set("parentComponent", "OscPanel" + (i + 1));

    fd.setContentData({
        "Type": "FilterDisplay",
        "ProcessorId": "OscFilter" + (i + 1),
        "Index": 0
    });

    // Shared LAF for consistent appearance across all filter displays
    fd.setLocalLookAndFeel(screenLaf);
}
```

```javascript
// Title: Markdown panel in a modal overlay framework
// Context: An in-app information dialog that displays formatted
// markdown text, with content updated dynamically

const var MARKDOWN_CONFIG = {
    "Type": "MarkdownPanel",
    "ShowBack": false,
    "ShowSearch": false,
    "ShowToc": false,
    "BoldFontName": "Default",
    "FixTocWidth": -1,
    "StartURL": "/",
    "CustomContent": "# Welcome\nThis is the default content."
};

const var infoTile = Content.addFloatingTile("InfoPanel", 100, 100);
infoTile.setPosition(100, 100, 600, 300);
infoTile.set("parentComponent", "ModalBackground");
infoTile.setContentData(MARKDOWN_CONFIG);

// Update the markdown text at runtime
inline function setInfoText(text)
{
    MARKDOWN_CONFIG.CustomContent = text;
    Content.getComponent("InfoPanel").setContentData(MARKDOWN_CONFIG);
}
```

**Pitfalls:**
- Every call to `setContentData()` destroys and recreates the entire embedded panel. In production code, gate calls behind a state-change check to avoid unnecessary rebuilds (e.g., only call when the `ProcessorId` actually changes).
- When reusing a shared JSON template across multiple `setContentData()` calls, always call `.clone()` before modifying the object. The original template is passed by reference, so mutations persist and affect subsequent uses.
- Colour properties set via `ft.set("bgColour", ...)` are mapped into the internal `ColourData` object. If `setContentData()` is called after setting colours, the colours may be overwritten by the new JSON data. Set colours after the content is established.
