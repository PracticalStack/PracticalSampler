Connects the slider to a processor parameter so modulation-related display and target metadata can be resolved for that parameter. This is mainly for modulation-aware UIs where the control should reflect matrix activity, not just raw value changes.

> [!Warning:Invalid moduleId clears display query] If the `moduleId` cannot be resolved, the method clears any previous modulation-display query for this slider.
