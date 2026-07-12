## hash

**Examples:**

```javascript:skip-redundant-loading
// Title: Skip redundant file loading by comparing state hashes
// Context: When multiple controls determine which audio file to load,
// concatenate their values into a string and hash it. Compare with
// the previous hash to avoid reloading when nothing changed.

const var Controls = [Content.getComponent("TypeSelector"),
                      Content.getComponent("VariantSelector"),
                      Content.getComponent("MicSelector")];

reg lastHash = 0;

inline function getSelectionHash()
{
    local s = "";
    
    for (c in Controls)
        s += c.getValue() + "_";
    
    return s.hash();
}

inline function loadIfChanged()
{
    local thisHash = getSelectionHash();
    
    if (thisHash == lastHash)
        return; // Nothing changed, skip the expensive load
    
    lastHash = thisHash;
    // ... load the audio file based on current selections
}
```
```json:testMetadata:skip-redundant-loading
{
  "testable": false,
  "skipReason": "Requires TypeSelector, VariantSelector, and MicSelector UI components"
}
```

```javascript:detect-pool-modification
// Title: Detect whether a list of items has been modified
// Context: Sum hashes of all items in a pool to detect changes
// without comparing every element individually.

var pool = ["Kick_01.wav", "Snare_02.wav", "HiHat_03.wav"];

inline function getPoolFingerprint(list)
{
    local sum = 0;
    
    for (n in list)
        sum += n.hash();
    
    return sum;
}

var before = getPoolFingerprint(pool);
pool.push("Clap_04.wav");
var after = getPoolFingerprint(pool);

Console.print(before == after); // 0 (false - pool changed)
```
```json:testMetadata:detect-pool-modification
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["0"]}
  ]
}
```
