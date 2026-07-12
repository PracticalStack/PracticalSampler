Sets a callback for tracking expansion installation progress. The callback receives a JSON object with the following properties:

| Property | Type | Description |
|---|---|---|
| `Status` | Integer | `-1` = not started, `0` = extraction starting, `1` = extracting, `2` = complete. The callback is guaranteed to fire with `2` at least once. |
| `Progress` | Double | Sample extraction progress (0.0-1.0). |
| `TotalProgress` | Double | Overall installation progress (0.0-1.0). |
| `SourceFile` | File | The `.hr` package file being extracted. |
| `TargetFolder` | File | The expansion root directory. |
| `SampleFolder` | File | The sample destination directory. |
| `Expansion` | Expansion | The created `Expansion` reference. Only valid when `Status` is `2`; `undefined` otherwise. |

The callback fires at three points: when extraction starts (`Status` 0), periodically during extraction at 300ms intervals (`Status` 1), and when installation completes (`Status` 2). Set it before calling `installExpansionFromPackage()`.

A typical post-installation workflow in the `Status == 2` handler:

```javascript
eh.setInstallCallback(function(obj)
{
    if (obj.Status == 2 && isDefined(obj.Expansion))
    {
        obj.Expansion.rebuildUserPresets();

        Engine.showYesNoWindow("Installation complete",
                               "Delete the archive file?",
        function(ok)
        {
            if (ok)
                packageFile.deleteFileOrDirectory();
        });

        eh.setCurrentExpansion(obj.Expansion);
    }
});
```
