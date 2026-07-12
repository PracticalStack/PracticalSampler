Sets a component property to the given value. During `onInit`, changes are applied without UI notification; outside `onInit`, the UI updates automatically.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Wrapper styling properties for the embedded web view component |
| `parentComponent` | Parent component for layout nesting |
| *`enableCache`*, *`enablePersistence`* | WebView data caching and persistent call replay behavior |
| *`scaleFactorToZoom`*, *`enableDebugMode`* | Browser zoom behavior and developer tools support |
