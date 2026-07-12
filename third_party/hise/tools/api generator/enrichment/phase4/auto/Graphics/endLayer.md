Ends the current layer, compositing its processed content back onto the parent canvas. Every `beginLayer()` or `beginBlendLayer()` call must be matched with a corresponding `endLayer()`. During compositing, the layer's draw actions are rendered to an offscreen image, any post-processing effects are applied, and the result is drawn onto the parent surface.

> [!Warning:Missing endLayer discards layer content] Forgetting to call `endLayer()` causes all subsequent draw calls to target the orphaned layer, and its content is silently discarded at the end of the paint callback.
