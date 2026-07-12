Sets the floating tile's content by providing a JSON configuration object. The `"Type"` property selects the panel type; additional properties configure colours, fonts, and panel-specific settings.

```javascript
ft.setContentData({
    "Type": "PresetBrowser",
    "Font": "Oxygen Bold",
    "FontSize": 16.0,
    "ColourData": {
        "bgColour": "0xFF222222",
        "textColour": "0xFFFFFFFF",
        "itemColour1": "0xFF444444"
    }
});
```

Available content types include `"PresetBrowser"`, `"Keyboard"`, `"PerformanceLabel"`, `"CustomSettings"`, `"AHDSRGraph"`, `"FilterDisplay"`, `"AudioAnalyser"`, `"MarkdownPanel"`, `"MatrixPeakMeter"`, `"TooltipPanel"`, `"MidiLearnPanel"`, `"ActivityLed"`, `"Plotter"`, `"Waveform"`, `"DraggableFilterPanel"`, `"WavetableWaterfall"`, `"MPEPanel"`, `"AboutPagePanel"`, `"MidiOverlayPanel"`, `"MidiSources"`, `"MidiChannelList"`, `"FrontendMacroPanel"`, and `"ModulationMatrix"`. Backend-only panel types are not available.

> [!Warning:Destroys and recreates entire panel] Calling this method destroys and recreates the entire panel. Avoid calling it at high frequency.

> [!Warning:Use itemColour1, not itemColour] Use `"itemColour1"` (not `"itemColour"`) in the `ColourData` sub-object. The base property `itemColour` maps to `"itemColour1"` internally.
