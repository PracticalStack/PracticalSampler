Controls whether the `object` parameter passed to `addListener` replaces the `this` reference inside the callback. The default is `true`, meaning `this` inside the callback refers to the object passed as the first argument to `addListener`.

Set to `false` when callbacks are methods on a script object and should access the object's own properties via `this`.
