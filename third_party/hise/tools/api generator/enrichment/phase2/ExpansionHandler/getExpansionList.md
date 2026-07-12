## getExpansionList

**Examples:**

```javascript:combobox-from-expansion-list
// Title: Build a ComboBox selector from available expansions
// Context: The most common pattern - populate a ComboBox with expansion names
// so the user can switch between them.

const var eh = Engine.createExpansionHandler();

// Build a name list with a "No Expansion" default entry
const var expansionNames = ["No Expansion"];

for (e in eh.getExpansionList())
    expansionNames.push(e.getProperties().Name);

const var ExpansionSelector = Content.getComponent("ExpansionSelector");
ExpansionSelector.set("items", expansionNames.join("\n"));
```
```json:testMetadata:combobox-from-expansion-list
{
  "testable": false,
  "skipReason": "Requires installed expansion packs and a pre-existing ComboBox UI component"
}
```

```javascript:aggregate-audio-files
// Title: Aggregate audio files from all expansions into a single selector
// Context: When a plugin has audio loop players that can load files from any
// expansion, collect all audio file references into one flat list.

const var eh = Engine.createExpansionHandler();
const var expansionList = eh.getExpansionList();

const var allNames = ["no file"];
const var allIds = [""];

// Add files from the project root
for (r in Engine.loadAudioFilesIntoPool())
{
    allNames.push(r.split("}")[1]);
    allIds.push(r);
}

// Add files from each expansion
for (e in expansionList)
{
    for (af in e.getAudioFileList())
    {
        allNames.push(af.split("}")[1]);
        allIds.push(af);
    }
}

const var FileSelector = Content.getComponent("FileSelector");
FileSelector.set("items", allNames.join("\n"));
```
```json:testMetadata:aggregate-audio-files
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with audio files and a pre-existing ComboBox UI component"
}
```

```javascript:two-column-expansion-browser
// Title: Two-column expansion and sample map browser
// Context: Build a browsable list where the left column shows expansions
// (plus "Root" for embedded maps) and the right column shows sample maps
// for the selected expansion.

const var eh = Engine.createExpansionHandler();
const var Sampler1 = Synth.getSampler("Sampler1");

// Get expansions and prepend undefined for the root/embedded maps
const var list = eh.getExpansionList();
list.insert(0, undefined);

// For each entry, collect its sample maps
for (entry in list)
{
    if (isDefined(entry))
    {
        local name = entry.getProperties().Name;
        local maps = entry.getSampleMapList();
        // ... populate UI with name and maps
    }
    else
    {
        // Root entry: show embedded sample maps
        local maps = Sampler.getSampleMapList();
        // ... populate UI with "Root" label and maps
    }
}
```
```json:testMetadata:two-column-expansion-browser
{
  "testable": false,
  "skipReason": "Requires installed expansion packs and a Sampler module; example contains pseudocode placeholders"
}
```
