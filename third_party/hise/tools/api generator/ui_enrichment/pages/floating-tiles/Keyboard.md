---
title: "Keyboard"
description: "Virtual MIDI keyboard with multiple rendering modes, per-key colouring, and configurable key range."
contentType: "Keyboard"
componentType: "floating-tile"
screenshot: "/images/v2/reference/ui-components/floating-tiles/keyboard.png"
llmRef: |
  Keyboard (FloatingTile)
  ContentType string: "Keyboard"
  Set via: FloatingTile.set("ContentType", "Keyboard")

  A virtual MIDI keyboard that sends and displays note events. Supports vector graphics, flat style, custom key images, CSS styling, MPE mode, and per-key colouring via Engine.setKeyColour().

  JSON Properties:
    CustomGraphics: Use custom key images from Images/keyboard/ (default: false)
    KeyWidth: Width per key in pixels (default: 14)
    LowKey: Lowest visible MIDI note number (default: 9)
    HiKey: Highest visible MIDI note number (default: 127)
    BlackKeyRatio: Height ratio of black keys to total height (default: 0.7)
    DefaultAppearance: Use standard HISE appearance with fixed height (default: true)
    DisplayOctaveNumber: Show octave labels on C keys (default: false)
    ToggleMode: Keys stay pressed until clicked again (default: false)
    MidiChannel: MIDI channel 1-16 (default: 1)
    MPEKeyboard: Enable MPE-style multi-touch keyboard (default: false)
    MPEStartChannel: MPE start channel (default: 2)
    MPEEndChannel: MPE end channel (default: 16)
    UseVectorGraphics: Use vector rendering instead of filmstrip (default: true)
    UseFlatStyle: Use flat style when vector graphics are active (default: false)

  Customisation:
    LAF: drawKeyboardBackground, drawWhiteNote, drawBlackNote
    CSS: .keyboard, .whitekey, .blackkey (requires CustomGraphics=false, DefaultAppearance=false)
seeAlso: []
commonMistakes:
  - title: "CSS not rendering on the keyboard"
    wrong: "Assigning a CSS stylesheet without disabling CustomGraphics and DefaultAppearance"
    right: "Set CustomGraphics to false and DefaultAppearance to false before applying CSS"
    explanation: "The CSS renderer is only active when both CustomGraphics and DefaultAppearance are disabled. With DefaultAppearance enabled, the keyboard uses fixed dimensions and the stock HISE skin."
  - title: "Keyboard stuck at fixed 72px height"
    wrong: "Trying to resize the keyboard height while DefaultAppearance is true"
    right: "Set DefaultAppearance to false to allow the keyboard to fill its parent bounds"
    explanation: "When DefaultAppearance is true, the keyboard is locked to a fixed 72px height centred in its container. Set it to false for fully resizable layout."
  - title: "MidiChannel does not isolate sound generators"
    wrong: "Setting MidiChannel to 2 expecting only the sampler on channel 2 to respond"
    right: "Use Message.ignoreEvent(true) in a MIDI processor on each child synth that should not respond to the keyboard"
    explanation: "The MidiChannel property controls which channel the keyboard sends notes on and which channel's activity it displays — it does not filter MIDI routing. The keyboard injects MIDI at the root of the processing chain, so all sound generators receive the events regardless of channel."
  - title: "No key colours or hover with CustomGraphics"
    wrong: "Enabling CustomGraphics and expecting Engine.setKeyColour() or hover highlighting to work"
    right: "Use LAF or CSS for custom visuals that require key colouring or hover feedback"
    explanation: "CustomGraphics mode renders static filmstrip images and does not support Engine.setKeyColour() overlays or mouse-over highlighting. Switch to LAF (drawWhiteNote/drawBlackNote) or CSS styling for full interactivity."
---

![Keyboard](/images/v2/reference/ui-components/floating-tiles/keyboard.png)

The Keyboard floating tile displays a virtual MIDI keyboard that sends and visualises note events. It supports multiple rendering modes: vector graphics (default), a flat style variant, custom filmstrip images, and full CSS styling. Individual keys can be coloured using `Engine.setKeyColour()`.

When MPE mode is enabled, the keyboard switches to a multi-touch layout suitable for MPE controllers with per-note pitch bend and slide.

## Setup

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "Keyboard");
ft.set("Data", JSON.stringify({
    "KeyWidth": 18.0,
    "LowKey": 24,
    "HiKey": 127,
    "BlackKeyRatio": 0.7,
    "DefaultAppearance": false,
    "DisplayOctaveNumber": true,
    "MidiChannel": 1
}));
```

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

### Appearance

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `CustomGraphics` | bool | `false` | Use custom key images from `{IMAGE_FOLDER}/Images/keyboard/` (files: `up_0.png` to `up_11.png`, `down_0.png` to `down_11.png`) |
| `DefaultAppearance` | bool | `true` | Use the standard HISE appearance (fixed 72px height, centred). Set to `false` for custom sizing. |
| `UseVectorGraphics` | bool | `true` | Use vector-based rendering instead of the legacy filmstrip keyboard |
| `UseFlatStyle` | bool | `false` | Use a flat visual style (only applies when `UseVectorGraphics` is `true`) |
| `DisplayOctaveNumber` | bool | `false` | Show octave labels (e.g. "C2") on each C key |

> [!Warning:CustomGraphics disables key colouring and hover] When `CustomGraphics` is enabled, `Engine.setKeyColour()` has no effect and mouse-over highlighting is absent. If you need per-key colouring (e.g. for keyswitch displays or playable range indicators), use LAF or CSS styling instead.

### Range & Size

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `KeyWidth` | int | `14` | Width of each white key in logical pixels |
| `LowKey` | int | `9` | Lowest visible key as MIDI note number |
| `HiKey` | int | `127` | Highest visible key as MIDI note number |
| `BlackKeyRatio` | float | `0.7` | Height of black keys as a proportion of the total keyboard height |

### Interaction

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ToggleMode` | bool | `false` | Keys stay pressed until clicked again |
| `MidiChannel` | int | `1` | MIDI channel for note events (1–16) |

> [!Warning:MidiChannel is display and send only] `MidiChannel` controls which channel the keyboard *sends* notes on and which channel's note activity it *displays*. It does **not** filter MIDI routing — the keyboard injects events at the root of the processing chain, so all sound generators receive them. If a user's hardware keyboard transmits on a channel other than 1, the floating tile will appear unresponsive.

### MPE

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `MPEKeyboard` | bool | `false` | Enable MPE-style multi-touch keyboard |
| `MPEStartChannel` | int | `2` | First MIDI channel for MPE zone |
| `MPEEndChannel` | int | `16` | Last MIDI channel for MPE zone |

The `ColourData` object can be used to set colours for the flat style rendering:

| Colour ID | Description |
|-----------|-------------|
| `bgColour` | Background colour |
| `itemColour1` | Key overlay colour |
| `itemColour2` | Top line colour |
| `itemColour3` | Activity indicator colour |

## LAF Customisation

Register a custom look and feel to control the rendering of each part of the keyboard.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawKeyboardBackground` | Draws the background behind all keys |
| `drawWhiteNote` | Draws a single white key (called per white key) |
| `drawBlackNote` | Draws a single black key (called per black key) |

### `obj` Properties

`drawKeyboardBackground` receives only the bounds:

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The keyboard bounds |

`drawWhiteNote` and `drawBlackNote` share identical properties:

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The key bounds |
| `obj.noteNumber` | int | MIDI note number (0–127) |
| `obj.hover` | bool | Whether the mouse is over the key |
| `obj.down` | bool | Whether the key is pressed |
| `obj.keyColour` | int (ARGB) | Custom key colour set via `Engine.setKeyColour()` |

### Example

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

// Draw the background
laf.registerFunction("drawKeyboardBackground", function(g, obj)
{
    g.setColour(0xFF222222);
    g.fillRect(obj.area);
});

// Draw each white key
laf.registerFunction("drawWhiteNote", function(g, obj)
{
    // Base colour — highlight if pressed
    g.setColour(obj.down ? 0xFFDDDDFF : 0xFFEEEEFF);
    g.fillRoundedRectangle([obj.area[0] + 1, obj.area[1] + 1, obj.area[2] - 2, obj.area[3] - 2], 2.0);
    
    // Apply per-key colour overlay
    if (obj.keyColour != 0)
    {
        g.setColour(Colours.withAlpha(obj.keyColour, 0.4));
        g.fillRect(obj.area);
    }
    
    // Hover highlight
    if (obj.hover)
    {
        g.setColour(0x10000000);
        g.fillRect(obj.area);
    }
    
    // Draw C note labels
    if (obj.noteNumber % 12 == 0)
    {
        g.setColour(0xFF666666);
        g.setFont("Arial", 10.0);
        var octave = Math.floor(obj.noteNumber / 12) - 2;
        g.drawAlignedText("C" + octave, obj.area, "centredBottom");
    }
});

// Draw each black key
laf.registerFunction("drawBlackNote", function(g, obj)
{
    g.setColour(obj.down ? 0xFF444466 : 0xFF222222);
    g.fillRoundedRectangle(obj.area, 2.0);
    
    // Apply per-key colour overlay
    if (obj.keyColour != 0)
    {
        g.setColour(Colours.withAlpha(obj.keyColour, 0.3));
        g.fillRect(obj.area);
    }
    
    if (obj.hover)
    {
        g.setColour(0x15FFFFFF);
        g.fillRect(obj.area);
    }
});

ft.setLocalLookAndFeel(laf);
```

## CSS Styling

The keyboard supports full CSS styling for highly customised key appearances including animations, shadows, and pseudo-3D effects. CSS rendering requires specific property configuration.

### Setup Requirements

1. Set `CustomGraphics` to `false`
2. Set `DefaultAppearance` to `false` (allows resizing and enables CSS)
3. Configure `KeyWidth` and `BlackKeyRatio` for desired proportions

### Selectors

| Selector | Description |
|----------|-------------|
| `.keyboard` | The background behind all keys |
| `.whitekey` | Each white key |
| `.blackkey` | Each black key |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the key |
| `:active` | Key is pressed down |

### Pseudo-elements

Both `.whitekey` and `.blackkey` support `::before` and `::after` pseudo-elements for multi-layered key rendering (e.g. colour overlays, 3D surface effects).

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--keyColour` | Per-key colour from `Engine.setKeyColour()` — `transparent` if not set |
| `--noteName` | Note label for C keys (e.g. `"C2"`), empty string for other keys |

### Example Stylesheet

This JSON property set works well with CSS:

```javascript
{
  "KeyWidth": 18.0,
  "DisplayOctaveNumber": false,
  "LowKey": 24,
  "HiKey": 127,
  "CustomGraphics": false,
  "DefaultAppearance": false,
  "BlackKeyRatio": 0.6,
  "ToggleMode": false,
  "MidiChannel": 1,
  "UseVectorGraphics": true,
  "UseFlatStyle": false,
  "MPEKeyboard": false,
  "MPEStartChannel": 2,
  "MPEEndChannel": 16
}
```

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
/** Background behind the keys. */
.keyboard
{
    background: #222;
}

/** White key styling. */
.whitekey
{
    content: var(--noteName);
    background: #eef;
    margin: 5% 5%;
    margin-bottom: 10%;
    border-radius: 2px;
    transition: margin 0.1s;
    transition: border-color 0.1s;
    box-shadow: none;
    vertical-align: bottom;
    font-size: 10px;
    padding-bottom: 3px;
    color: #666;
    border: 1px solid rgba(255, 255, 255, 0.7);
}

/** Colour overlay for keys set via Engine.setKeyColour(). */
.whitekey::after
{
    content: '';
    background-color: color-mix(in rgb, var(--keyColour) 60%, transparent);
    border-radius: 2px;
}

.whitekey:hover
{
    background: white;
}

/** Bottom element for a pseudo-3D effect. */
.whitekey::before
{
    content: '';
    background: linear-gradient(to bottom, #333, #000);
    position: absolute;
    bottom: 0px;
    height: 8%;
    margin: 5%;
    border-radius: 15%;
    transition: bottom 0.05s;
}

/** Pressed state — shifts down with inset shadow. */
.whitekey:active
{
    margin-bottom: 5%;
    background-color: linear-gradient(to bottom, #eef, #dde);
    box-shadow: inset 0px 2px 10px rgba(0, 0, 0, 0.3);
    border-color: transparent;
}

.whitekey::before:active
{
    bottom: -5%;
    background-color: #111;
}

/** Black key styling. */
.blackkey
{
    background: #222;
    border-radius: 5%;
    border-width: 10px 4px;
    border-color: #222;
    border-style: solid;
    transition: margin-top 0.1s;
    margin: 10px;
    box-shadow: 0px 3% 10% rgba(0, 0, 0, 0.5);
}

.blackkey:active
{
    margin-top: 12px;
}

/** Top surface of the black key for a 3D effect. */
.blackkey::before
{
    content: '';
    margin: 20%;
    margin-bottom: 20%;
    margin-top: 3%;
    transition: margin-bottom 0.1s;
    transition: background-color 0.1s;
    background: linear-gradient(to top, #363636, #282828);
    border-radius: 5%;
}

.blackkey::before:active
{
    background: linear-gradient(to top, #404040, #363636);
    margin-bottom: 6px;
}

/** Colour overlay for black keys. */
.blackkey::after
{
    content: '';
    margin: 1px;
    background-color: color-mix(in rgb, var(--keyColour) 20%, transparent);
    border-radius: 2px;
}
");

ft.setLocalLookAndFeel(laf);
```

## Notes

- Set per-key colours using `Engine.setKeyColour(noteNumber, colour)`. These colours are available as `obj.keyColour` in LAF callbacks and `var(--keyColour)` in CSS.
- The CSS renderer adds a 10px margin to the black key area to allow shadows to render without being clipped by the key bounding box.
- When `DefaultAppearance` is `true`, the keyboard is locked to a fixed 72px height centred within a standard container width. Set it to `false` for fully resizable layout.
- `CustomGraphics` mode expects 24 image files in `{IMAGE_FOLDER}/Images/keyboard/`: `up_0.png` to `up_11.png` (released keys) and `down_0.png` to `down_11.png` (pressed keys), one per chromatic note.
- The `MidiChannel` property determines which MIDI channel the keyboard sends note events on. In MPE mode, the `MPEStartChannel` and `MPEEndChannel` properties define the MPE zone.

> [!Tip:Selective MIDI routing with multiple sound generators] The keyboard sends MIDI into the root of the processing chain, reaching every child synth. To prevent unwanted triggering (e.g. an Audio Loop Player responding to keyboard clicks), add a Script Processor to the child synth and call `Message.ignoreEvent(true)` in its `onNoteOn` and `onNoteOff` callbacks.

**See also:** <!-- populated during cross-reference post-processing -->
