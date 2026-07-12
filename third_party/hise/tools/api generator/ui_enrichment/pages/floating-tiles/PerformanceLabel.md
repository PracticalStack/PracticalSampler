---
title: "PerformanceLabel"
description: "Real-time display of CPU usage, memory consumption, and active voice count."
contentType: "PerformanceLabel"
componentType: "floating-tile"
screenshot: "/images/v2/reference/ui-components/floating-tiles/performancelabel.png"
llmRef: |
  PerformanceLabel (FloatingTile)
  ContentType string: "PerformanceLabel"
  Set via: FloatingTile.set("ContentType", "PerformanceLabel")

  Displays real-time CPU usage, memory consumption, and voice count for the plugin. No module connection required — reads global engine statistics.

  JSON Properties:
    Font: Font family name
    FontSize: Font size in points
    ColourData.textColour: Text colour
    ColourData.bgColour: Background colour

  Customisation:
    LAF: none
    CSS: none

seeAlso: []
commonMistakes:
  - title: "Expecting per-module statistics"
    wrong: "Using PerformanceLabel to monitor CPU usage of a specific processor"
    right: "Use Engine.getCpuUsage() for global stats, or the HISE profiler for per-module analysis"
    explanation: "PerformanceLabel shows global engine performance (total CPU, memory, voice count), not per-processor metrics. There is no floating tile for per-module CPU monitoring."
---

![PerformanceLabel](/images/v2/reference/ui-components/floating-tiles/performancelabel.png)

The PerformanceLabel floating tile displays real-time system statistics for the plugin: CPU usage, memory consumption, and active voice count. It provides a quick at-a-glance performance overview, useful during development and optionally in the final plugin UI.

Unlike most floating tiles, the PerformanceLabel does not connect to a specific module — it reads global engine statistics automatically.

## Setup

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "PerformanceLabel");
ft.set("Data", JSON.stringify({
    "Font": "Oxygen Bold",
    "FontSize": 18,
    "ColourData": {
        "textColour": "0xFFEEEEEE"
    }
}));
```

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Font` | String | `"Default"` | Font family name |
| `FontSize` | double | `14` | Font size in points |

The `ColourData` object supports these colour IDs:

| Colour ID | Description |
|-----------|-------------|
| `textColour` | Text colour for the statistics labels |
| `bgColour` | Background colour |

## Notes

- The **CPU** value shows the percentage of the audio buffer time used for processing. For example, 30% means that filling a 512-sample buffer (roughly 10 ms at 44.1 kHz) takes about 3 ms.
- For scripting access to the individual values displayed by this panel, use `Engine.getCpuUsage()`, `Engine.getMemoryUsage()`, and `Engine.getNumVoices()`.
- This content type has no LAF functions and no CSS support. To create a fully custom performance display, read the values via the Engine scripting calls above and render them in a ScriptPanel paint routine.

**See also:** <!-- populated during cross-reference post-processing -->
