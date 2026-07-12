UserPresetHandler::setPluginParameterGroupNames(Array pluginParameterGroupNames) -> undefined

Thread safety: INIT -- runtime calls throw a script error
Registers valid plugin parameter group names for use in setCustomAutomation slot
definitions (pluginParameterGroup property). Must be called before
setCustomAutomation if any slots use non-empty group names. Groups organize
DAW-visible parameters into named categories (some hosts display as folders).
Anti-patterns:
  - Passing a non-array value throws a script error ("pluginParameterGroupNames
    must be an array of strings"). Individual elements are silently converted
    via toString().
Pair with:
  setCustomAutomation -- validated against registered group names
  setPluginParameterSortFunction -- sort function receives group name per parameter
Source:
  ScriptExpansion.cpp  setPluginParameterGroupNames()
    -> stores StringArray on UserPresetHandler::pluginParameterGroups
    -> validated by checkPluginParameterGroupName() during setCustomAutomation
