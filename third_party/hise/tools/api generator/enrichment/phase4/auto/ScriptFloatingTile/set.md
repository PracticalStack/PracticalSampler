Sets a component property to the given value. ScriptFloatingTile adds several properties beyond the standard set: `ContentType` (panel type string), `Font`, `FontSize`, `Data` (JSON configuration), `updateAfterInit`, and `itemColour3`. Many inherited properties like `saveInPreset`, `macroControl`, `min`, `max`, `text`, and `tooltip` are deactivated.

Reports a script error if the property does not exist. During `onInit`, changes are applied silently; outside `onInit`, change notifications update the UI.
