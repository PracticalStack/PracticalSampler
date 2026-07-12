Adds a value to the stack if it is not already present (set semantics). Returns true if the value was added, false if it was a duplicate or the stack is full. In float mode, pass a number. In event mode, pass a MessageHolder.

> [!Warning:Silent failure at 128-element capacity] Silently returns false when the stack reaches its 128-element capacity. No error is reported, so check the return value if you need to detect overflow.
