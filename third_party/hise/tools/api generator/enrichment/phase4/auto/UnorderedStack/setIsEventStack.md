Switches the stack between float mode (default) and event mode. The second parameter sets the compare function used by `contains()`, `remove()`, and `removeIfEqual()` for event matching - pass one of the built-in constants or a custom inline function that receives two MessageHolder arguments and returns true for a match.

> [!Warning:Set mode once during initialisation] Set the mode once during initialisation. Switching modes does not clear the previous mode's data, which can leave stale elements in the backing array.
