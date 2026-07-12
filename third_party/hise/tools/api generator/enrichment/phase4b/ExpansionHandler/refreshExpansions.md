ExpansionHandler::refreshExpansions() -> bool

Thread safety: UNSAFE -- filesystem scan, heap allocations for new expansion objects.
Rescans the expansion folder for new or changed expansions. Discovers new directories,
creates expansion objects, sorts alphabetically. Triggers the expansion callback if
new expansions are found. Returns true on success.
Dispatch/mechanics:
  ExpansionHandler::createAvailableExpansions()
    -> scans Expansions/ folder for child directories
    -> skips already-known expansions
    -> createExpansionForFile() for each new folder -> initialise()
    -> sorts alphabetically -> sends ExpansionCreated notification
Pair with:
  getExpansionList -- retrieve the updated list after refresh
  setExpansionCallback -- receive notifications about newly discovered expansions
Source:
  ScriptExpansion.cpp  refreshExpansions()
    -> ExpansionHandler::createAvailableExpansions() (ExpansionHandler.cpp:208)
