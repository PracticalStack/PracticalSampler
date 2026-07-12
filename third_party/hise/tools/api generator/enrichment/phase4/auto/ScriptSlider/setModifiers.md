Assigns a modifier gesture to one action key from the modifiers object created by `createModifiers()`. Configure these mappings once during `onInit`, then reuse the same modifiers object across slider groups to keep interaction behaviour consistent.

Use bitwise OR for alternative modifiers (`mods.rightClick | mods.altDown`) and arrays for required combinations (`[mods.doubleClick, mods.shiftDown]`, up to three entries). Use `mods.noKeyModifier` when you need a gesture only when no keyboard modifier is held.

> [!Warning:Avoid overlapping gesture mappings] Gesture collisions are resolved by the first internal match, and matching order is not stable. Avoid overlapping mappings with the default map unless you redefine the conflicting actions explicitly.

> [!Warning:Use noKeyModifier not noKeyModifiers] Use `mods.noKeyModifier` (singular). Some older examples show `noKeyModifiers`, which does not match the current constant name.
