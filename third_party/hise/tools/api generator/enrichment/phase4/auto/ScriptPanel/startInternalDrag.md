Initiates an internal HISE drag operation from this panel. The `dragData` object can contain any properties - use a `"Type"` property to distinguish different drag operations in a complex UI. Drop targets receive this data through their mouse callback's drag events.

The panel's `allowCallbacks` property must be set to `"Clicks, Hover & Dragging"` or higher to support drag interaction.
