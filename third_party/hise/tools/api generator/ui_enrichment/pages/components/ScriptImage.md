---
title: "Image"
description: "Lightweight static image display with alpha blending, filmstrip offset, and blend modes from the project image pool."
componentId: "ScriptImage"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/image.png"
llmRef: |
  ScriptImage (UI component)
  Create via: Content.addImage("name", x, y)
  Scripting API: $API.ScriptImage$

  Static image display component. Shows a single image from the project's image pool
  with optional filmstrip offset, alpha blending, and Photoshop-style blend modes.

  Properties (component-specific):
    fileName: source image path from the project image pool
    alpha: opacity (0.0 to 1.0)
    offset: vertical pixel offset for filmstrip-style multi-frame images
    scale: image scaling factor
    blendMode: one of 24 Photoshop-style blend modes (gin library)
    allowCallbacks: mouse callback level for click interaction
    popupMenuItems: right-click context menu entries
    popupOnRightClick: enable right-click popup trigger

  Customisation:
    LAF: none
    CSS: img (minimal — basic positioning/sizing only)
    Filmstrip: yes (via offset property with vertically-stacked image)

seeAlso: []
commonMistakes:
  - title: "Using ScriptPanel for static images"
    wrong: "Using ScriptPanel with loadImage() + setPaintRoutine() to display a static image"
    right: "Use ScriptImage — it avoids paint callback overhead and is purpose-built for static image display"
    explanation: "ScriptPanel's paint routine runs a scripting callback on every repaint. ScriptImage renders the image directly without any callback overhead."
  - title: "Expecting forceUseRealFile to bypass the pool"
    wrong: "Passing forceUseRealFile=true to setImageFile() expecting a direct disk reload"
    right: "Images always load through the pool/expansion handler regardless of this parameter"
    explanation: "The forceUseRealFile parameter is ignored in the implementation (marked ignoreUnused in C++)."
  - title: "Image missing after export"
    wrong: "Setting fileName to a path that works in the HISE IDE but not in the compiled plugin"
    right: "Use the {PROJECT_FOLDER} wildcard: set(\"fileName\", \"{PROJECT_FOLDER}myImage.png\")"
    explanation: "Images must be referenced via the project image pool using {PROJECT_FOLDER}. Absolute or ad-hoc paths work in the IDE but fail in exported plugins because the image pool embeds files from the project Images folder."
---

![Image](/images/v2/reference/ui-components/image.png)

ScriptImage is a lightweight component for displaying static images from the project's image pool. It is the simplest plugin component in HISE and the preferred choice for background artwork, logos, decorative overlays, and any visual element that does not require dynamic drawing.

For basic use, set the `fileName` property in the Interface Designer — no scripting is needed. For dynamic scenarios such as switching images based on expansion selection or UI state, use `set("fileName", path)` at runtime. ScriptImage also supports vertical filmstrip-style images via the `offset` property, which shifts the visible portion of a multi-frame stacked image.

The component supports 24 Photoshop-style blend modes (provided by the gin library), adjustable opacity via `alpha`, and optional mouse interaction through `allowCallbacks` for click-responsive image areas.

## Properties

Set properties with `ScriptImage.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`fileName`* | String | `""` | Path to the source image from the project image pool. Use the `{PROJECT_FOLDER}` wildcard for project-relative paths (e.g. `{PROJECT_FOLDER}myImage.png`). |

> [!Tip:Images are stored uncompressed in memory] HISE decodes images to raw ARGB pixels in memory regardless of PNG compression. A 400x3000 PNG always consumes ~4.7 MB in RAM. Keep total image sizes modest (under 15 MB) to avoid memory issues when multiple plugin instances are loaded simultaneously.
| *`alpha`* | double | `1.0` | Image opacity from `0.0` (fully transparent) to `1.0` (fully opaque). |
| *`offset`* | int | `0` | Vertical pixel offset into the source image. Used with filmstrip-style images where multiple frames are stacked vertically in a single file. |
| *`scale`* | double | `1.0` | Image scaling factor. |
| *`blendMode`* | String | `"Normal"` | Photoshop-style blend mode for compositing with underlying content. One of 24 modes from the gin library. |

> [!Warning:BlendMode is currently non-functional] The ScriptImage blend mode implementation is broken — the blend function requires a source image to composite against which the rendering logic does not supply. For image compositing effects, use a ScriptPanel with `loadImage()` and manual `drawImage()` calls, or layer images with CSS `mix-blend-mode` if available.
| *`allowCallbacks`* | String | `"No Callbacks"` | Mouse callback level. Controls whether and which mouse events the image responds to, enabling click-responsive image areas. |

> [!Warning:Overlay images block mouse interaction on components below] Even with `allowCallbacks` set to `"No Callbacks"`, a ScriptImage placed on top of other components will intercept mouse events and prevent interaction with controls underneath. To use an image as a decorative overlay (e.g. a vignette or shadow), set `enabled` to `false` — this makes the image transparent to mouse events while still rendering visually.
| *`popupMenuItems`* | String | `""` | Newline-separated list of items for a right-click context menu. |
| *`popupOnRightClick`* | bool | `true` | When enabled, shows the popup menu on right-click. |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `tooltip` | Hover tooltip text |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `isMetaParameter` | DAW automation and meta-parameter support |
| `processorId`, `parameterId` | Module parameter connection |

### Deactivated properties

The following properties are deactivated for ScriptImage and have no effect:

`bgColour`, `itemColour`, `itemColour2`, `min`, `max`, `defaultValue`, `textColour`, `macroControl`, `automationID`, `linkedTo`, `text`.

## CSS Styling

ScriptImage has minimal CSS support. The `img` HTML tag selector is registered in the CSS renderer, but CSS styling options are limited to basic positioning and sizing. For visual effects such as opacity, blend modes, and filmstrip offsets, use the component properties directly.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `img` | HTML tag | Selects all ScriptImage elements |
| `#ImageId` | ID | Targets a specific image component by name |

## Notes

- **Prefer ScriptImage over ScriptPanel for static images.** Using `ScriptPanel` with `loadImage()` + `setPaintRoutine()` to display a static image is unnecessary overhead. ScriptImage is purpose-built for this and avoids the paint callback.
- **`saveInPreset` defaults to `false`** for ScriptImage. This is correct for most uses — background images and decorative elements should not be saved in presets.
- **Use `set("fileName", path)` rather than `setImageFile()`** for consistency with the standard property API. Both achieve the same result.
- **Prefer the Interface Designer** for setting images when the image is static. Only use scripting when the image needs to change at runtime.
- The `forceUseRealFile` parameter on `setImageFile()` is ignored — images always load through the pool/expansion handler.

**See also:** {placeholder — populated during cross-reference post-processing}
