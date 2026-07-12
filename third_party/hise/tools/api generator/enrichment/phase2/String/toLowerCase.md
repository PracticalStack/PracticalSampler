## toLowerCase

**Examples:**

```javascript:case-insensitive-categorization
// Title: Case-insensitive keyword search for auto-categorization
// Context: Automatically assign audio files to categories based on
// keywords in the file name. Convert to lowercase once, then check
// multiple keywords to classify the sound.

inline function getCategoryFromName(fileName)
{
    local lower = fileName.toLowerCase();
    
    if (lower.indexOf("kick") != -1 || lower.indexOf("bass") != -1)
        return "Low";
    
    if (lower.indexOf("snare") != -1 || lower.indexOf("clap") != -1)
        return "Mid";
    
    if (lower.indexOf("hat") != -1 || lower.indexOf("shaker") != -1)
        return "High";
    
    return "Other";
}

Console.print(getCategoryFromName("Hard_KICK_01")); // Low
Console.print(getCategoryFromName("Bright-Hat"));    // High
```
```json:testMetadata:case-insensitive-categorization
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Low", "High"]}
  ]
}
```
