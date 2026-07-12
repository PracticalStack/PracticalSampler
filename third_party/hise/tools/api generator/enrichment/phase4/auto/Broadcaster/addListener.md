Adds a general-purpose callback listener that fires whenever the broadcaster sends a message. The `object` parameter becomes the `this` reference inside the callback by default and is also used for identification when removing listeners. On registration, the callback immediately receives the broadcaster's current values.

Listeners are sorted by metadata priority (higher values execute first). Pass a JSON object with a `priority` field to control execution order.

> [!Warning:Object parameter replaces this reference] The `object` parameter replaces `this` inside the callback by default. If you need the original `this` scope, call `setReplaceThisReference(false)` before adding listeners. Arrays and objects passed as `object` are held by reference - if modified after registration, the callback sees the modified version.
