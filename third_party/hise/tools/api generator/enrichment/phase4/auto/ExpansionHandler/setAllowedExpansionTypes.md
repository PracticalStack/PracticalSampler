Restricts which expansion types can be loaded. Pass an array of expansion type constants (`ExpansionHandler.FileBased`, `ExpansionHandler.Intermediate`, `ExpansionHandler.Encrypted`). Expansions whose type is not in the allowed list are moved to the uninitialised list and become invisible to `getExpansionList()`.

```javascript
// Only allow encrypted expansions in production
eh.setAllowedExpansionTypes([eh.Encrypted]);
```

> [!Warning:Different type lists for development and release] During development you typically need `FileBased` expansions; in release builds you likely want only `Intermediate` or `Encrypted`. Make sure the type list matches your build context, otherwise expansions that work in the IDE will not appear in the exported plugin.
