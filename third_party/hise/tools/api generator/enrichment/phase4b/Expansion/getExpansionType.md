Expansion::getExpansionType() -> int

Thread safety: UNSAFE -- for Intermediate/Encrypted expansions, checks file existence on disk to determine type
Returns the expansion type: 0 (FileBased), 1 (Intermediate), or 2 (Encrypted).
Returns -1 if the expansion reference has been invalidated.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  Expansion::getExpansionTypeFromFolder(root)
    -> checks info.hxp (Encrypted) -> info.hxi (Intermediate) -> expansion_info.xml (FileBased)
    -> priority order: Encrypted > Intermediate > FileBased

Anti-patterns:
  - Do NOT assume return value is always 0-2 -- returns -1 when the expansion has been
    unloaded or deleted. Always check against ExpansionHandler constants.

Source:
  ExpansionHandler.cpp:708  Expansion::getExpansionTypeFromFolder()
    -> Helpers::getExpansionInfoFile(f, type).existsAsFile() for each type in priority order
