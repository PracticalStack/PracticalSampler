Removes this expansion from the active expansion list. After calling, the expansion no longer appears in `ExpansionHandler.getExpansionList()` until the next application restart or re-initialisation.

> [!Warning:Invalidates the current reference] After calling this method, the Expansion object becomes invalid. Any subsequent method call on it will throw a script error. Do not store or reuse the reference after unloading.
