Stores a value using reference semantics. For arrays and objects, the storage shares the reference with the caller rather than copying the data.

> [!Warning:Shared references cause race conditions] If you continue to modify an array or object after calling `store()`, readers on other threads see the mutations mid-flight. Use `storeWithCopy()` when the source data will be modified after storing.