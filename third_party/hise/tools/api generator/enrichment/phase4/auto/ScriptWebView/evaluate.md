Evaluates arbitrary JavaScript code in the webview. The `identifier` parameter serves as a key for the persistence system - when `enablePersistence` is true, the code is stored and re-evaluated when new webview instances are created. Reusing the same identifier overwrites the previously stored code for that key.

> [!Warning:Identifier must be unique per operation] Each identifier must be unique per logical operation. Reusing an identifier overwrites the previous code in the persistence system, which may cause unexpected behaviour on webview re-initialisation.
