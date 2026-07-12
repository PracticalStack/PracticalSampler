## getScreenBounds

**Examples:**

```javascript:drag-to-zoom-handler
// Title: Implementing a drag-to-zoom handler with screen bounds clamping
// Context: Commercial plugins implement zoom by letting the user drag a corner
// panel. getScreenBounds provides the display height needed to calculate the
// maximum safe zoom level that won't exceed the screen.

Content.makeFrontInterface(900, 600);

const var zoomPanel = Content.addPanel("ZoomPanel", 850, 550);
zoomPanel.set("width", 50);
zoomPanel.set("height", 50);

namespace ZoomHandler
{
    const var MIN_ZOOM = 0.75;
    const var MAX_ZOOM = 2.0;
    const var ZOOM_STEP = 0.25;
    const var INTERFACE_HEIGHT = 600;

    zoomPanel.setMouseCallback(function(event)
    {
        if (event.clicked)
            this.data.zoomStart = Settings.getZoomLevel();

        if (event.drag)
        {
            var currentZoom = Settings.getZoomLevel();

            // Compute drag delta as a fraction of interface height
            local dragPixel = (event.dragY * currentZoom) / INTERFACE_HEIGHT;

            // Clamp to screen height using getScreenBounds
            // false = user area only (excludes taskbar)
            local maxZoom = Content.getScreenBounds(false)[3] / INTERFACE_HEIGHT;

            local newZoom = this.data.zoomStart + dragPixel;

            // Snap to step increments
            newZoom += (ZOOM_STEP / 2);
            newZoom = Math.min(newZoom, maxZoom);
            newZoom -= Math.fmod(newZoom, ZOOM_STEP);
            newZoom = Math.range(newZoom, MIN_ZOOM, MAX_ZOOM);

            if (currentZoom != newZoom)
                Settings.setZoomLevel(newZoom);
        }
    });
}
```
```json:testMetadata:drag-to-zoom-handler
{
  "testable": false,
  "skipReason": "Mouse drag callback requires user interaction and Settings.setZoomLevel has visual side-effects"
}
```
