<!-- Diagram triage:
  - No diagram specs in Phase 1 data
-->

# ScriptImage

ScriptImage is a UI component for displaying static images from the project's image pool. Create one with `Content.addImage()` or retrieve an existing one with `Content.getComponent()`:

```js
const var img = Content.addImage("MyImage", 0, 0);

// Or retrieve an existing component
const var img = Content.getComponent("MyImage");
```

The component supports three main capabilities:

1. **Image display** with adjustable opacity via the `alpha` property and 24 Photoshop-style blend modes via `blendMode`.
2. **Filmstrip animation** using the `offset` property to select frames from a vertically-stacked image strip - useful for BPM displays, step indicators, and LED-style status lights.
3. **Popup menus** via the `popupMenuItems` and `popupOnRightClick` properties, where the selected menu item index becomes the component's value.

Images are loaded from the project's `Images/` folder using pool references with the `{PROJECT_FOLDER}` prefix. Expansion-aware plugins can swap artwork dynamically by updating the `fileName` property when the active expansion changes.

For static artwork that never changes at runtime, set the `fileName` property directly in the Interface Designer rather than through scripting. Use `ScriptPanel` instead when you need custom drawing, mouse interaction beyond popup menus, or animation.

> [!Tip:saveInPreset defaults to false] The `saveInPreset` property defaults to `false` for ScriptImage, which is correct for most uses - background images and decorative elements should not be saved in user presets.

## Common Mistakes

- **Use ScriptImage not Panel for static images**
  **Wrong:** Creating a `ScriptPanel` with `loadImage()` and `setPaintRoutine()` just to display a static image.
  **Right:** Using `ScriptImage` with the `fileName` property set in the Interface Designer.
  *ScriptImage is purpose-built for static image display. ScriptPanel adds unnecessary overhead (paint callback, mouse handling infrastructure) when no custom drawing or interaction is needed.*

- **forceUseRealFile has no effect**
  **Wrong:** `img.setImageFile("myImage.png", true)` expecting disk reload via the second parameter.
  **Right:** `img.setImageFile("{PROJECT_FOLDER}myImage.png", 0)`
  *The `forceUseRealFile` parameter is ignored in the current implementation - images are always loaded through the pool. Use the standard `{PROJECT_FOLDER}` pool reference format.*
