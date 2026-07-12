Registers a paint function that receives a [Graphics](/scripting/scripting-api/graphics) object for custom drawing. The function executes asynchronously on the scripting thread when `repaint()` is called - not on the calling thread.

Store shared drawing helpers as functions on the panel's `data` object so that multiple panels can reuse the same paint logic without duplicating code. Inside the paint routine, `this` refers to the panel instance, so use `this.getWidth()`, `this.getHeight()`, and `this.data` to access dimensions and state.

> [!Warning:setImage and setPaintRoutine are exclusive] Calling `setImage()` clears the paint routine and switches to fixed-image mode. These two rendering approaches are mutually exclusive on a single panel.
