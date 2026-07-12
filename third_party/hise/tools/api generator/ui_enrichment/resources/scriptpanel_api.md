# ScriptPanel — Scripting API Enrichment Backport

**Purpose:** Content extracted from the old `scriptpanel.md` reference guide that belongs in the **scripting API enrichment** (phase4/phase4b). This covers callback systems, programming patterns, data model, and best practices that are method-level or programming-level documentation.

**Action required:** Review against existing phase4b ScriptPanel docs and backport any missing information. Items already covered in phase4b are marked `[COVERED]`. Items with new information are marked `[GAP]`.

**Source:** `ui_enrichment/resources/scriptpanel.md` (old docs)

---

## 1. Paint Routine (`setPaintRoutine`)

**phase4b status:** `[COVERED]` — Basic signature and mechanics documented. 

**Gap: Graphics API tutorial content.** The old docs contain a substantial tutorial on using the Graphics object inside the paint routine:

- **Colours:** 32-bit hex format `0xAARRGGBB`, `Colours` constants, `Colours.withAlpha(colour, alpha)`
- **Areas/Rectangles:** Array format `[x, y, width, height]`, relative positioning via `this.getWidth()` / `this.getHeight()`
- **Images:** `loadImage("{PROJECT_FOLDER}image.png", "name")` then `g.drawImage("name", area, xOffset, yOffset)`. Images scale to fit width. Retina support via double-resolution images drawn at half size. Filmstrip support via y-offset parameter. Images are reference-shared between panels.
- **Fonts:** `Engine.loadFont("{PROJECT_FOLDER}Fonts/font.ttf")` then `g.setFont("Font Name", size)`. System fonts available in IDE, embedded from Images folder in compiled plugins. Bold/Italic via appending ` Bold` or ` Italic` to font name. OS-specific font names via `Engine.getOS()`.
- **Paths:** `Content.createPath()` for vector shapes. Methods: `startNewSubPath()`, `lineTo()`, `quadraticTo()`, `closeSubPath()`, `addArc()`, `loadFromData()` (SVG path data). Paths scale to given area. Use normalized 0..1 range during creation, then `getBounds(scale)` for sizing.

**Recommendation:** This content enriches the `Graphics` class documentation and/or the `setPaintRoutine` method docs. The Graphics class already has its own phase4b entry — check if these practical usage patterns are covered there. The image loading, font loading, and path creation patterns are particularly valuable.

> **Note on canvas behaviour:** "When you change the size of the panel, it will not be re-rendered, but the current canvas will be stretched (until you call `Panel.repaint()`)" — This is a non-obvious gotcha worth including in `setPaintRoutine` docs.

---

## 2. Mouse Event Callback (`setMouseCallback`)

**phase4b status:** `[COVERED]` — Event properties table and anti-patterns documented.

**Gap: Callback level semantics.** The old docs have a detailed table of which events fire at each `allowCallbacks` level. The phase4b `setMouseCallback.md` mentions the requirement but doesn't include the full level-by-event matrix:

| Property | Availability |
|----------|-------------|
| `event.result`, `event.itemText` | `"Context Menu"` and above |
| `event.mouseDownX`, `event.mouseDownY` | `"Clicks Only"` and above |
| `event.clicked`, `event.doubleClick`, `event.rightClick`, `event.mouseUp` | `"Clicks Only"` and above |
| `event.hover` | `"Clicks & Hover"` and above |
| `event.drag`, `event.dragX`, `event.dragY` | `"Clicks, Hover & Dragging"` and above |
| `event.x`, `event.y` | `"All Callbacks"` only |

**Modifier keys** (available at all levels):
- `event.shiftDown` — Shift
- `event.cmdDown` — Ctrl (Win) / Cmd (Mac)
- `event.ctrlDown` — Ctrl on both platforms
- `event.altDown` — Alt (Win) / Apple Key (Mac)

**Recommendation:** The event-by-level matrix is valuable reference material that should be included in the `setMouseCallback` phase4b docs or phase4a user docs. The modifier key table with platform differences is also useful.

> **Note on hover:** "Clicks and all other events will set `event.hover` to `1`, so you might want to handle them separately." — This is a subtle gotcha worth documenting.

---

## 3. Timer Callback (`setTimerCallback`)

**phase4b status:** `[COVERED]` — Basic mechanics documented.

**Gap: Millisecond vs seconds.** The old docs note: "the `startTimer()` argument is in **milliseconds** (as opposed to `Synth.startTimer()`, which is in seconds)." This platform inconsistency is worth flagging in the docs if not already present.

**No other gaps identified.** The phase4b docs cover the timer callback adequately.

---

## 4. Data Storage Model

**phase4b status:** `[PARTIALLY COVERED]` — The `Readme.md` mentions `data = {}` constant and `this.data` usage in callbacks.

**Gap: UI Data vs Control Data distinction.** The old docs make a clear conceptual distinction that would benefit the API docs:

### UI Data (`this.data`)
- Non-persistent object for internal rendering/state (hover flags, animation state, cached calculations)
- Populated with arbitrary properties: `panel.data.alphaValue = 1.0`
- **Critical rule:** Inside callbacks, access via `this.data`, NOT via the external variable name. This enables factory methods where multiple panels share the same callback code but have independent state.
- Values stored in `data` are **always non-persistent** even if `saveInPreset` is `true`
- Prefer simple numbers over complex types for performance

### Control Data (`setValue` / `getValue`)
- Persistent data representing the widget's value (restored from presets)
- Can be any type: number, boolean, array, object (but NOT strings directly — wrap in object if needed)
- `setValue(value)` does NOT fire the `onControl` callback — call `changed()` explicitly
- For array values (e.g., ButtonPack), handle size mismatches on preset restore

**Recommendation:** The `this.data` vs `setValue`/`getValue` distinction and the persistence rules should be in the ScriptPanel API docs (possibly in the `Readme.md` overview or as a note in `setValue.md`).

---

## 5. Performance Best Practices

**phase4b status:** `[PARTIALLY COVERED]` — Readme mentions timer intervals and `repaint()` optimization.

**Gap: Additional performance tips from old docs:**

1. **`opaque` property:** Setting to `true` prevents parent repainting. Already in `scriptpanel_ui.md` as a property, but the performance implication should also be in the API docs.

2. **Repaint rate:** 30-50ms timer interval is sufficient for most animations (20-30 fps). Higher rates don't improve fluidity but clog the message event system. `[COVERED in Readme]`

3. **`repaintImmediately()`:** Bypasses the internal event queue for more fluid animations. Only safe to call from timer or mouse callbacks — NOT from MIDI callbacks or `onControl`. `[PARTIALLY COVERED]`

4. **Conditional repainting:** Only call `repaint()` when the displayed value actually changed. `[COVERED in Readme]`

---

## 6. Factory Method Pattern

**phase4b status:** `[NOT COVERED]`

**Content from old docs:**

The factory method pattern is the standard approach for creating reusable ScriptPanel widgets:

```javascript
inline function createMyWidget(name, x, y)
{
    local widget = Content.addPanel(name, x, y);
    
    widget.data.someState = 0;
    
    widget.setPaintRoutine(function(g)
    {
        // Use this.data, not widget.data
        g.fillAll(Colours.withAlpha(Colours.white, this.data.someState));
    });
    
    return widget;
};

const var Panel = createMyWidget("Panel", 0, 0);
```

Key points:
- Use `createXXX` naming convention — this enables the Interface Designer to recognize and reposition the widget via drag-and-drop
- Use `this` keyword inside callbacks for per-panel state access
- Use `local` for the widget variable inside the factory, `const var` for the returned reference
- HISE can auto-generate factory methods from panel selections (right-click → "Create UI Factory method from selection")

**Recommendation:** This is a general HISEScript pattern, but it's so central to ScriptPanel usage that it warrants mention in the ScriptPanel API overview. A brief note + cross-reference would suffice.

---

## 7. Namespace Encapsulation Pattern

**phase4b status:** `[NOT COVERED]`

**Content from old docs:**

For encapsulated, reusable widgets, put each widget in its own namespace with at least two public functions:

```javascript
namespace MyFunkyPanel
{
    inline function createMyFunkyPanel(name, x, y)
    { ... };

    inline function handleUpdate(panel, newValue)
    {
        panel.setValue(newValue);
        panel.startTimer(50);
        panel.repaint();
    };
};
```

Use leading underscores for "private" methods (`_getButton`, `_toggleButton`).

**Recommendation:** This is a general HISEScript pattern. A brief mention in the ScriptPanel overview with a cross-reference to a HISEScript style guide would be appropriate. Not critical for the API enrichment.

---

## 8. Worked Examples

**phase4b status:** `[NOT APPLICABLE]` — Examples belong in snippets or tutorial pages, not API docs.

The old docs contain four worked examples:
1. **Six-state filmstrip button** — Panel as a filmstrip button with hover/press states
2. **ButtonPack** — Panel as an array of toggle buttons with drag support
3. **Infinitely rotatable head** — Panel as a filmstrip-based infinite rotary control
4. **Vectorized knob** — Panel as a vector-drawn knob with arc value display

**Recommendation:** These are candidates for the snippet library if not already present. The six-state button snippet already exists (`sixstate-filmstrip-button`). The ButtonPack and vector knob may be worth adding as new snippets. They should NOT be included in the API docs or UI component page.

---

## Summary: What to backport

| Content | Target | Priority |
|---------|--------|----------|
| Graphics API tutorial (images, fonts, paths) | `Graphics` class phase4a/4b or `setPaintRoutine` docs | Medium — check if already in Graphics docs |
| Event-by-callback-level matrix | `setMouseCallback` phase4a/4b | High — useful reference not in current docs |
| Modifier keys with platform differences | `setMouseCallback` phase4a/4b | Medium |
| `hover` gotcha (always 1 during clicks) | `setMouseCallback` phase4a/4b | Low — subtle but useful |
| `startTimer()` is milliseconds (vs `Synth.startTimer()` seconds) | `setTimerCallback` or `startTimer` docs | Low — check if already noted |
| UI Data vs Control Data model | ScriptPanel `Readme.md` or `setValue` docs | High — fundamental concept |
| Canvas stretch on resize gotcha | `setPaintRoutine` docs | Low |
| Factory method pattern | ScriptPanel `Readme.md` (brief mention) | Low |
| Worked examples | Snippet library candidates | Low — separate effort |
