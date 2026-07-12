Updates custom automation values in one of two modes depending on the first argument:

- **Array mode:** Pass an array of `{"id": "...", "value": ...}` objects to set specific automation slot values. The output of `createObjectForAutomationValues` uses exactly this format, making snapshot/restore round-trips straightforward.
- **Integer mode:** Pass an integer to refresh all automation slots from their connected processor parameters. The integer specifies which connection index to read from (0 for the primary connection).

The second argument controls notification dispatch (`SyncNotification`, `AsyncNotification`, or `false` to suppress notifications). The third argument enables undo integration when true.

> [!Warning:Wrap single objects in array brackets] The array mode expects an Array, not a single object. Passing `{"id": "Vol", "value": 0.5}` without wrapping it in `[...]` throws a script error.
