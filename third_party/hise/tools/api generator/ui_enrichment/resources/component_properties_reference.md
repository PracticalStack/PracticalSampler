# Component Properties Reference

**Purpose:** Authoritative property list per UI component type, sourced directly from the HISE runtime (2026-03-28). Use this as the single source of truth for property names, types, defaults, and available options.

**Important:** Property IDs are case-sensitive. Most component-specific properties use camelCase. The few PascalCase exceptions are noted explicitly.

---

## Common Properties (all ScriptComponents)

These properties exist on every component type. Default values may vary per component.

| Property | Type | Description |
|----------|------|-------------|
| `text` | String | Display text (defaults to component ID) |
| `visible` | bool | Display state |
| `enabled` | bool | Interaction state |
| `locked` | bool | Prevents editing in Interface Designer |
| `x`, `y` | int | Position in pixels, relative to parent |
| `width`, `height` | int | Size in pixels |
| `min` | double | Minimum value |
| `max` | double | Maximum value |
| `defaultValue` | double | Default/initial value |
| `tooltip` | String | Hover tooltip text |
| `bgColour` | hex String | Background colour |
| `itemColour` | hex String | Item colour 1 |
| `itemColour2` | hex String | Item colour 2 |
| `textColour` | hex String | Text colour |
| `macroControl` | int | Macro control slot (-1 = none) |
| `saveInPreset` | bool | Preset persistence |
| `isPluginParameter` | bool | DAW automation exposure |
| `pluginParameterName` | String | DAW parameter display name |
| `pluginParameterGroup` | String | DAW parameter group |
| `deferControlCallback` | bool | Defer callback to message thread |
| `isMetaParameter` | bool | Meta-parameter flag |
| `linkedTo` | String | Mirror another component's value |
| `automationID` | String | Automation identifier (**note: capital ID**) |
| `useUndoManager` | bool | Undo support |
| `parentComponent` | String | Parent component for nesting |
| `processorId` | String | Connected module ID |
| `parameterId` | String | Connected parameter ID |

---

## ScriptButton

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `filmstripImage` | String | `""` | Load new File, Use default skin | Filmstrip image path |
| `numStrips` | String | `"2"` | — | Number of filmstrip frames |
| `isVertical` | bool | `true` | — | Filmstrip orientation |
| `scaleFactor` | double | `1` | — | Filmstrip scale |
| `radioGroup` | int | `0` | — | Radio group ID (0 = none) |
| `isMomentary` | int | `0` | — | Momentary button mode |
| `enableMidiLearn` | bool | `true` | — | MIDI learn support |
| `setValueOnClick` | bool | `false` | — | Set value on click (vs toggle) |
| `mouseCursor` | String | `"ParentCursor"` | ParentCursor, NoCursor, NormalCursor, WaitCursor, IBeamCursor, CrosshairCursor, CopyingCursor, PointingHandCursor, DraggingHandCursor, LeftRightResizeCursor, UpDownResizeCursor, UpDownLeftRightResizeCursor, TopEdgeResizeCursor, BottomEdgeResizeCursor, LeftEdgeResizeCursor, RightEdgeResizeCursor, TopLeftCornerResizeCursor, TopRightCornerResizeCursor, BottomLeftCornerResizeCursor, BottomRightCornerResizeCursor | Mouse cursor style |

**Deactivated:** `min`, `max` (button value is always 0 or 1)

**Default overrides:** `saveInPreset` = `true`

---

## ScriptSlider

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `mode` | String | `"Linear"` | Frequency, Decibel, Time, TempoSync, Linear, Discrete, Pan, NormalizedPercentage | Value mode |
| `style` | String | `"Knob"` | Knob, Horizontal, Vertical, Range | Visual style |
| `stepSize` | double | `0.01` | 0.0, 0.01, 0.1, 1.0 | Value step size |
| `middlePosition` | double | `-1` | — | Middle position for skewed display |
| `suffix` | String | `""` | — | Value suffix string |
| `filmstripImage` | String | `"Use default skin"` | Load new File, Use default skin | Filmstrip image path |
| `numStrips` | int | `0` | — | Number of filmstrip frames |
| `isVertical` | bool | `true` | — | Filmstrip orientation |
| `scaleFactor` | double | `1` | — | Filmstrip scale |
| `mouseSensitivity` | double | `1` | — | Mouse drag sensitivity |
| `dragDirection` | String | `"Diagonal"` | Diagonal, Vertical, Horizontal | Drag direction |
| `showValuePopup` | String | `"No"` | No, Above, Below, Left, Right | Value popup display |
| `showTextBox` | bool | `false` | — | Show text box below slider |
| `scrollWheel` | bool | `true` | — | Enable scroll wheel control |
| `enableMidiLearn` | bool | `true` | — | MIDI learn support |
| `sendValueOnDrag` | bool | `true` | — | Send value during drag (vs on release) |
| `matrixTargetId` | String | `""` | — | Matrix modulation target ID |

**Default overrides:** `saveInPreset` = `true`

---

## ScriptComboBox

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `items` | String | `""` | — | Newline-separated item list |
| `fontName` | String | `"Default"` | (system fonts) | Font family |
| `fontSize` | double | `13` | — | Font size |
| `fontStyle` | String | `"plain"` | Regular, Italic, Bold, Bold Italic | Font style |
| `enableMidiLearn` | bool | `false` | — | MIDI learn support |
| `popupAlignment` | String | `"bottom"` | bottom, top, topRight, bottomRight | Popup menu alignment |
| `useCustomPopup` | bool | `false` | — | Enable advanced popup syntax |

**Default overrides:** `saveInPreset` = `true`, `min` = `1`, `defaultValue` = `1`

---

## ScriptLabel

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `fontName` | String | `"Arial"` | (system fonts) | Font family |
| `fontSize` | double | `13` | — | Font size |
| `fontStyle` | String | `"plain"` | Regular, Italic, Bold, Bold Italic, Password | Font style |
| `alignment` | String | `"centred"` | left, right, top, bottom, centred, centredTop, centredBottom, topLeft, topRight, bottomLeft, bottomRight | Text alignment |
| `editable` | bool | `false` | — | Allow text editing |
| `multiline` | bool | `false` | — | Multi-line text mode |
| `updateEachKey` | bool | `false` | — | Fire callback on each keystroke |

**Default overrides:** `saveInPreset` = `false`

---

## ScriptPanel

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `borderSize` | double | `2` | Border width in pixels |
| `borderRadius` | double | `6` | Border corner radius |
| `opaque` | bool | `false` | Skip parent repaint when true |
| `allowDragging` | int | `0` | Enable drag behavior |
| `allowCallbacks` | String | `"No Callbacks"` | Mouse callback level (Options: No Callbacks, Context Menu, Clicks Only, Clicks & Hover, Clicks Hover & Dragging, All Callbacks) |
| `popupMenuItems` | String | `""` | Context menu items |
| `popupOnRightClick` | bool | `true` | Show popup on right-click |
| `popupMenuAlign` | bool | `false` | Align popup to panel bounds |
| `selectedPopupIndex` | int | `-1` | Currently selected popup item |
| `stepSize` | double | `0` | Value step size |
| `enableMidiLearn` | bool | `false` | MIDI learn support |
| `holdIsRightClick` | bool | `true` | Long press triggers right-click |
| `isPopupPanel` | bool | `false` | Popup panel mode |
| `bufferToImage` | bool | `false` | Cache paint output as bitmap |

**Default overrides:** `saveInPreset` = `false`

---

## ScriptTable

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `tableIndex` | int | `0` | Table slot index on connected processor |
| `customColours` | hex String | `"0x00000000"` | Enable flat design with custom colours |

**Deactivated:** `min`, `max`, `defaultValue`, `textColour`, `parameterId`, `macroControl`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`

**Default overrides:** `saveInPreset` = `true`

---

## ScriptSliderPack

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `sliderAmount` | int | `16` | — | Number of sliders |
| `stepSize` | double | `0.01` | 0.01, 0.1, 1.0 | Value step size |
| `flashActive` | bool | `true` | — | Flash active slider step |
| `showValueOverlay` | bool | `true` | — | Show value text overlay |
| `SliderPackIndex` | int | `0` | — | Slider pack data slot index (**PascalCase exception**) |
| `mouseUpCallback` | bool | `false` | — | Fire callback only on mouse up |
| `stepSequencerMode` | bool | `false` | — | Step sequencer interaction mode |

**Default overrides:** `saveInPreset` = `true`, `defaultValue` = `1`

---

## ScriptAudioWaveform

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `itemColour3` | hex String | `"0x22FFFFFF"` | Additional accent colour |
| `opaque` | bool | `true` | Skip parent repaint |
| `showLines` | bool | `false` | Show horizontal guide lines |
| `showFileName` | bool | `true` | Show filename text overlay |
| `sampleIndex` | int | `0` | Audio file slot index |
| `enableRange` | bool | `true` | Enable range selection |
| `loadWithLeftClick` | bool | `false` | Load file on left click |

**Deactivated:** `macroControl`, `parameterId`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`, `text`, `min`, `max`, `defaultValue`

**Default overrides:** `saveInPreset` = `true`

---

## ScriptImage

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `alpha` | double | `1` | — | Opacity (0.0–1.0) |
| `fileName` | String | `""` | Load new File | Image path from project pool |
| `offset` | int | `0` | — | Vertical filmstrip offset |
| `scale` | double | `1` | — | Image scale factor |
| `blendMode` | String | `"Normal"` | Normal, Lighten, Darken, Multiply, Average, Add, Subtract, Difference, Negation, Screen, Exclusion, Overlay, SoftLight, HardLight, ColorDodge, ColorBurn, LinearDodge, LinearBurn, LinearLight, VividLight, PinLight, HardMix, Reflect, Glow, Phoenix | Blend mode |
| `allowCallbacks` | bool/String | `false` | No Callbacks, Context Menu, Clicks Only, Clicks & Hover, Clicks Hover & Dragging, All Callbacks | Mouse callback level |
| `popupMenuItems` | String | `""` | — | Context menu items |
| `popupOnRightClick` | bool | `true` | — | Show popup on right-click |

**Deactivated:** `bgColour`, `itemColour`, `itemColour2`, `min`, `max`, `defaultValue`, `textColour`, `macroControl`, `automationID`, `linkedTo`, `text`

**Default overrides:** `saveInPreset` = `false`

---

## ScriptedViewport

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `scrollBarThickness` | int | `16` | — | Scrollbar width |
| `autoHide` | bool | `true` | — | Auto-hide scrollbar |
| `useList` | bool | `false` | — | List mode (vs plain viewport) |
| `viewPositionX` | int | `0` | — | Horizontal scroll position |
| `viewPositionY` | int | `0` | — | Vertical scroll position |
| `items` | String | `""` | — | Newline-separated list items |
| `fontName` | String | `"Arial"` | (system fonts) | Font family |
| `fontSize` | double | `13` | — | Font size |
| `fontStyle` | String | `"plain"` | Regular, Italic, Bold, Bold Italic | Font style |
| `alignment` | String | `"centred"` | left, right, top, bottom, centred, centredTop, centredBottom, topLeft, topRight, bottomLeft, bottomRight | Text alignment |

**Default overrides:** `saveInPreset` = `true`

---

## ScriptFloatingTile

| Property | Type | Default | Options | Description |
|----------|------|---------|---------|-------------|
| `itemColour3` | hex String | `"0x00000000"` | — | Additional colour |
| `updateAfterInit` | bool | `true` | — | Update content after init |
| `ContentType` | String | `"Empty"` | Empty, PresetBrowser, AboutPagePanel, Keyboard, PerformanceLabel, MidiOverlayPanel, ActivityLed, CustomSettings, MidiSources, MidiChannelList, TooltipPanel, MidiLearnPanel, FrontendMacroPanel, Plotter, AudioAnalyser, Waveform, FilterDisplay, DraggableFilterPanel, WavetableWaterfall, MPEPanel, ModulationMatrix, ModulationMatrixController, AHDSRGraph, FlexAHDSRGraph, MarkdownPanel, MatrixPeakMeter | Content type (**PascalCase**) |
| `Font` | String | `"Default"` | (system fonts) | Font family (**PascalCase**) |
| `FontSize` | double | `14` | — | Font size (**PascalCase**) |
| `Data` | String | `"{\n}"` | — | JSON configuration data (**PascalCase**) |

**Default overrides:** `saveInPreset` = `false`

**Note:** ScriptFloatingTile is the only component where `Font`, `FontSize`, `ContentType`, and `Data` use PascalCase. This is because these properties are registered by the FloatingTile panel system, not the ScriptComponent base.

---

## ScriptDynamicContainer

No component-specific properties. Uses only common properties.

**Default overrides:** `saveInPreset` = `true`

---

## PascalCase Exceptions Summary

Only these properties genuinely use PascalCase (all others are camelCase):

| Component | Property | Notes |
|-----------|----------|-------|
| All | `automationID` | Capital "ID" at end |
| ScriptFloatingTile | `ContentType` | FloatingTile panel system |
| ScriptFloatingTile | `Font` | FloatingTile panel system |
| ScriptFloatingTile | `FontSize` | FloatingTile panel system |
| ScriptFloatingTile | `Data` | FloatingTile panel system |
| ScriptSliderPack | `SliderPackIndex` | Historical exception |
