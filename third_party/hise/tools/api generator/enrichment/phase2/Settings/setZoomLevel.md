## setZoomLevel

**Examples:**

```javascript:drag-to-zoom-handle
// Title: Drag-to-zoom corner handle
// Context: A ScriptPanel in the bottom-right corner of the interface
// lets the user drag to resize. Zoom snaps to step increments and
// clamps to screen bounds.

const var MIN_ZOOM = 0.85;
const var MAX_ZOOM = 2.0;
const var ZOOM_STEP = 0.25;
const var INTERFACE_WIDTH = 900;
const var INTERFACE_HEIGHT = 600;

const var ZoomPanel = Content.getComponent("ZoomPanel");

ZoomPanel.setMouseCallback(function(event)
{
    if(event.clicked)
    {
        this.data.zoomStart = Settings.getZoomLevel();
    }

    if(event.drag)
    {
        var currentZoom = Settings.getZoomLevel();
        var dragPixel = 0;

        // Use whichever axis has more drag distance
        if(event.dragX > event.dragY)
            dragPixel = (event.dragX * currentZoom) / INTERFACE_WIDTH;
        else
            dragPixel = (event.dragY * currentZoom) / INTERFACE_HEIGHT;

        // Clamp to screen bounds
        var maxScaleFactor = Content.getScreenBounds(false)[3] / INTERFACE_HEIGHT;
        var newZoom = this.data.zoomStart + dragPixel;

        // Snap to step increments (0.25)
        newZoom += (ZOOM_STEP / 2);
        newZoom = Math.min(newZoom, maxScaleFactor);
        newZoom -= Math.fmod(newZoom, ZOOM_STEP);
        newZoom = Math.range(newZoom, MIN_ZOOM, MAX_ZOOM);

        if(currentZoom != newZoom)
            Settings.setZoomLevel(newZoom);
    }
});
```

```json:testMetadata:drag-to-zoom-handle
{
  "testable": false,
  "skipReason": "Requires mouse drag interaction on a ScriptPanel"
}
```

```javascript:combobox-zoom-selector
// Title: ComboBox zoom selector with discrete levels
// Context: A settings menu provides a dropdown with predefined zoom
// percentages. On init, the current zoom level is reflected in the
// ComboBox selection.

const var ZOOM_LEVELS = [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0];

const var cmbZoom = Content.getComponent("cmbZoom");

inline function onZoomControl(component, value)
{
    Settings.setZoomLevel(ZOOM_LEVELS[value - 1]);
};

cmbZoom.setControlCallback(onZoomControl);

// Sync the ComboBox to the current zoom on init
inline function refreshZoom(zoomToUse)
{
    local zIdx = 0;

    for(v in ZOOM_LEVELS)
    {
        if(zoomToUse < v)
            break;

        zIdx++;
    }

    cmbZoom.setValue(zIdx);
    Settings.setZoomLevel(zoomToUse);
}

refreshZoom(Settings.getZoomLevel());
```

```json:testMetadata:combobox-zoom-selector
{
  "testable": false,
  "skipReason": "Requires UI ComboBox component and modifies global zoom state"
}
```

**Pitfalls:**
- Always clamp zoom to screen bounds before calling `setZoomLevel`. Use `Content.getScreenBounds(false)[3] / interfaceHeight` as the practical maximum - the Settings 2.0 ceiling alone does not prevent the interface from exceeding the display.
