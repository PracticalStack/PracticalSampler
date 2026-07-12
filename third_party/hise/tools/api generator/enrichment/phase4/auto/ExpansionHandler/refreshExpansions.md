Rescans the expansion folder for new or changed expansions. Discovers new expansion directories, creates expansion objects for them, and sorts the list alphabetically. Also retries initialisation of previously failed expansions (for example, encrypted expansions that could not load because credentials were missing at startup). Returns `true` on success.

You only need to call this manually when implementing a custom installation routine. It is called automatically in these situations:

1. The user enters licence credentials (causes a reload of all encrypted expansions with the new credentials)
2. `encodeWithCredentials()` completes (refreshes to pick up the newly encoded expansion)
3. `installExpansionFromPackage()` completes (refreshes to pick up the installed expansion)
