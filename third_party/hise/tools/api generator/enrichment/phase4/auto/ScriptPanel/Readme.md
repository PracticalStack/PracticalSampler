<!-- Diagram triage: no diagrams specified in Phase 1 data -->

# ScriptPanel

ScriptPanel is the most versatile UI component in HISE, providing a blank canvas for custom drawing, mouse interaction, and dynamic behaviour. Created with `Content.addPanel(name, x, y)`, it serves purposes ranging from simple layout containers to fully interactive custom controls.

ScriptPanel supports six main capabilities:

1. Custom drawing via paint routines that receive a Graphics object
2. Mouse interaction at configurable callback levels (from clicks only to full mouse tracking)
3. Periodic timer callbacks for polling and animation
4. File drag-and-drop handling (both inbound drops and outbound file drags)
5. Parent-child panel hierarchies for dynamic UI creation at runtime
6. Lottie animation playback (requires rLottie)

The `data` property provides a persistent object for storing arbitrary per-panel state. Use `this.data` inside callbacks to keep state co-located with the panel rather than in external variables.

```js
const var pnl = Content.addPanel("MyPanel", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);
```

> [!Tip:Set opaque and minimal allowCallbacks level] Set the `opaque` property to `true` on panels that fill their entire area with a solid background to avoid unnecessary alpha compositing. Use the minimum `allowCallbacks` level needed for your use case - `"All Callbacks"` fires on every mouse move, which adds overhead when only click handling is required.

## Common Mistakes

- **Set allowCallbacks before mouse callback**
  **Wrong:** `pnl.setMouseCallback(fn)` without setting `allowCallbacks`
  **Right:** Call `pnl.set("allowCallbacks", "Clicks Only")` before `setMouseCallback`
  *The `allowCallbacks` property defaults to "No Callbacks", so the mouse callback never fires unless explicitly enabled.*

- **Cache component references outside paint**
  **Wrong:** Calling `Content.getComponent()` inside `setPaintRoutine` or `setTimerCallback`
  **Right:** Cache component references in `const var` at init time
  *`Content.getComponent()` performs a lookup on every call. In a 30ms timer, this adds thousands of unnecessary lookups per minute.*

- **Use panel value store not external vars**
  **Wrong:** Using external `var` for panel state shared between callbacks
  **Right:** Use `this.data.propertyName` inside callbacks
  *The `data` object is per-panel and accessible via `this` in all callbacks. External variables create coupling and break when panels are created in loops.*

- **Only repaint when state changes**
  **Wrong:** Calling `repaint()` unconditionally in every timer tick
  **Right:** Compare the new value to the stored value and only call `repaint()` when it changes
  *Redundant repaints waste rendering cycles. A simple equality check before repainting avoids this.*
