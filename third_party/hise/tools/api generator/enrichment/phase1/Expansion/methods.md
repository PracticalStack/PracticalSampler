## getAudioFileList

**Signature:** `Array getAudioFileList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Calls loadAllFilesFromProjectFolder() which performs filesystem I/O, plus heap allocations for array and string construction.
**Minimal Example:** `var files = {obj}.getAudioFileList();`

**Description:**
Returns an array of pool reference strings for all audio files in this expansion. Forces discovery of all audio files by calling loadAllFilesFromProjectFolder() before querying the pool. Returns an empty array if the expansion has no audio files. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Pitfalls:**
- Unlike getSampleMapList and getMidiFileList, this method triggers a filesystem scan before listing. The first call may be slower than subsequent calls due to file discovery overhead.

**Cross References:**
- `$API.Expansion.getWildcardReference$`

## getDataFileList

**Signature:** `Array getDataFileList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Heap allocations for array and string construction.
**Minimal Example:** `var files = {obj}.getDataFileList();`

**Description:**
Returns an array of pool reference strings for all data files in this expansion's AdditionalSourceCode pool. Unlike getImageList and getAudioFileList, this method does not call loadAllFilesFromProjectFolder() -- it returns only files already known to the pool. Returns an empty array if no data files exist. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Cross References:**
- `$API.Expansion.loadDataFile$`
- `$API.Expansion.writeDataFile$`

## getExpansionType

**Signature:** `int getExpansionType()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** For Intermediate/Encrypted expansions, the underlying implementation checks file existence on disk to determine the type.
**Minimal Example:** `var type = {obj}.getExpansionType();`

**Description:**
Returns the expansion type as an integer matching the constants on ExpansionHandler: 0 (FileBased), 1 (Intermediate), or 2 (Encrypted). Returns -1 if the expansion reference has been invalidated. For Intermediate and Encrypted expansions, the type is determined at runtime by checking which info file exists in the expansion's root folder (info.hxp for Encrypted, info.hxi for Intermediate, expansion_info.xml for FileBased).

**Parameters:**
None.

**Pitfalls:**
- Returns -1 (not a valid ExpansionHandler type constant) when the expansion has been unloaded or deleted. Always check the return value against the ExpansionHandler constants rather than assuming a valid range.

**Cross References:**
- `$API.ExpansionHandler.FileBased$`
- `$API.ExpansionHandler.Intermediate$`
- `$API.ExpansionHandler.Encrypted$`
- `$API.Expansion.rebuildUserPresets$`

## getImageList

**Signature:** `Array getImageList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Calls loadAllFilesFromProjectFolder() which performs filesystem I/O, plus heap allocations for array and string construction.
**Minimal Example:** `var images = {obj}.getImageList();`

**Description:**
Returns an array of pool reference strings for all image files in this expansion. Forces discovery of all image files by calling loadAllFilesFromProjectFolder() before querying the pool. Returns an empty array if the expansion has no images. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Pitfalls:**
- Unlike getSampleMapList and getMidiFileList, this method triggers a filesystem scan before listing. The first call may be slower than subsequent calls due to file discovery overhead.

**Cross References:**
- `$API.Expansion.getWildcardReference$`

## getMidiFileList

**Signature:** `Array getMidiFileList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Heap allocations for array and string construction.
**Minimal Example:** `var midiFiles = {obj}.getMidiFileList();`

**Description:**
Returns an array of pool reference strings for all MIDI files in this expansion. Does not trigger filesystem discovery -- relies on the pool being populated during expansion initialisation. Returns an empty array if no MIDI files exist. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Cross References:**
- `$API.Expansion.getWildcardReference$`

## getProperties

**Signature:** `JSON getProperties()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a DynamicObject from the internal ValueTree (heap allocation, string construction).
**Minimal Example:** `var props = {obj}.getProperties();`

**Description:**
Returns a JSON object containing this expansion's metadata properties. Standard properties include Name (defaults to folder name), Version (defaults to "1.0.0"), Tags (defaults to empty string), ProjectName (from project settings), and ProjectVersion (from project settings). Additional properties from the expansion_info.xml may also be present depending on what the expansion author defined (Description, Company, CompanyURL, URL, UUID, etc.). Returns undefined if the expansion reference has been invalidated.

**Parameters:**
None.

**Example:**
```javascript:expansion-properties
// Title: Reading expansion metadata
const var eh = Engine.createExpansionHandler();
const var list = eh.getExpansionList();

if (list.length > 0)
{
    var props = list[0].getProperties();
    Console.print("Name: " + props.Name);
    Console.print("Version: " + props.Version);
}
```

```json:testMetadata:expansion-properties
{
  "testable": false,
  "skipReason": "Requires installed expansion packs in the project's Expansions folder"
}
```

## getRootFolder

**Signature:** `ScriptObject getRootFolder()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap allocation of ScriptFile object.
**Minimal Example:** `var root = {obj}.getRootFolder();`

**Description:**
Returns a File object pointing to the root folder of this expansion on disk. Throws a script error ("Expansion was deleted") if the expansion reference has been invalidated.

**Parameters:**
None.

**Cross References:**
- `$API.Expansion.getSampleFolder$`

## getSampleFolder

**Signature:** `ScriptObject getSampleFolder()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap allocation of ScriptFile object.
**Minimal Example:** `var samples = {obj}.getSampleFolder();`

**Description:**
Returns a File object pointing to the Samples subdirectory of this expansion. If a link file redirects the Samples folder to a different location, the resolved target path is returned.

**Parameters:**
None.

**Pitfalls:**
- [BUG] Does not check whether the expansion reference is still valid before accessing the internal expansion object. If the expansion has been unloaded, this may crash instead of throwing a descriptive script error like other methods do.

**Cross References:**
- `$API.Expansion.setSampleFolder$`
- `$API.Expansion.getRootFolder$`

## getSampleMapList

**Signature:** `Array getSampleMapList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Heap allocations for array and string construction.
**Minimal Example:** `var maps = {obj}.getSampleMapList();`

**Description:**
Returns an array of sample map reference strings for this expansion. The .xml extension is stripped from each reference string. Does not trigger filesystem discovery -- relies on the SampleMap pool being populated during expansion initialisation. Returns an empty array if no sample maps exist. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Cross References:**
- `$API.Expansion.getWildcardReference$`

## getUserPresetList

**Signature:** `Array getUserPresetList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Performs recursive filesystem scan for .preset files.
**Minimal Example:** `var presets = {obj}.getUserPresetList();`

**Description:**
Returns an array of user preset names for this expansion. Unlike other list methods, this scans the filesystem directly rather than querying a pool. Searches the UserPresets subdirectory recursively for .preset files. Returns relative paths from the UserPresets folder with the .preset extension stripped and backslashes normalized to forward slashes. Returns an empty array if no presets exist. Throws a script error if the expansion reference has been invalidated.

**Parameters:**
None.

**Cross References:**
- `$API.Expansion.rebuildUserPresets$`

## getWildcardReference

**Signature:** `String getWildcardReference(var relativePath)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String construction and concatenation involve atomic ref-count operations.
**Minimal Example:** `var ref = {obj}.getWildcardReference("SampleMaps/Piano.xml");`

**Description:**
Constructs a wildcard reference string by prepending this expansion's wildcard prefix to the given relative path. The result has the format `{EXP::ExpansionName}relativePath`. This reference can be used with pool-based APIs to address expansion-specific resources. Returns an empty string if the expansion reference has been invalidated.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePath | String | no | Relative path within the expansion | -- |

**Example:**
```javascript:wildcard-reference
// Title: Building a wildcard reference for an expansion resource
const var eh = Engine.createExpansionHandler();
const var list = eh.getExpansionList();

if (list.length > 0)
{
    var ref = list[0].getWildcardReference("SampleMaps/Piano.xml");
    Console.print(ref); // {EXP::MyExpansion}SampleMaps/Piano.xml
}
```

```json:testMetadata:wildcard-reference
{
  "testable": false,
  "skipReason": "Requires installed expansion packs in the project's Expansions folder"
}
```

**Cross References:**
- `$API.Expansion.getSampleMapList$`
- `$API.Expansion.getAudioFileList$`
- `$API.Expansion.getImageList$`
- `$API.Expansion.getMidiFileList$`
- `$API.Expansion.getDataFileList$`

## loadDataFile

**Signature:** `var loadDataFile(var relativePath)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** File I/O (FileBased path) or pool loading (Intermediate/Encrypted), plus JSON parsing and heap allocation.
**Minimal Example:** `var data = {obj}.loadDataFile("myConfig.json");`

**Description:**
Loads a JSON data file from this expansion's AdditionalSourceCode directory. Behavior differs by expansion type: for FileBased expansions, the file is read directly from the filesystem; for Intermediate/Encrypted expansions, the file is loaded from the embedded data pool using a wildcard reference with strong caching. Both paths parse the content as JSON and return the parsed object. Returns undefined if the file is not found (FileBased) or the pool reference cannot be resolved (Intermediate/Encrypted). Throws a script error if JSON parsing fails on the Intermediate/Encrypted path.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePath | String | no | Relative path to the JSON file within the AdditionalSourceCode folder | -- |

**Pitfalls:**
- On the FileBased code path, a missing file silently returns undefined with no error message. Check the return value with `isDefined()`.
- On the Intermediate/Encrypted path, a missing pool reference also silently returns undefined, but a JSON parse error throws a script error.

**Cross References:**
- `$API.Expansion.writeDataFile$`
- `$API.Expansion.getDataFileList$`

## rebuildUserPresets

**Signature:** `bool rebuildUserPresets()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** File I/O for ValueTree loading, potential decryption, and user preset file extraction to disk.
**Minimal Example:** `var ok = {obj}.rebuildUserPresets();`

**Description:**
Extracts user presets from the encoded expansion data to the filesystem. Only works with Intermediate or Encrypted expansion types (those backed by ScriptEncryptedExpansion in C++). Forces extraction even if user presets already exist on disk, overwriting them. Returns true on success. Outputs debug errors to the console if the ValueTree cannot be loaded (e.g., missing credentials for encrypted expansions).

**Parameters:**
None.

**Pitfalls:**
- Silently returns false for FileBased expansions with no error message. The internal dynamic_cast to ScriptEncryptedExpansion fails. Check the expansion type with getExpansionType() before calling.
- Overwrites existing user preset files when successful (forceExtraction = true). There is no way to merge or preserve user modifications to presets.

**Cross References:**
- `$API.Expansion.getExpansionType$`
- `$API.Expansion.getUserPresetList$`

## setAllowDuplicateSamples

**Signature:** `void setAllowDuplicateSamples(var shouldAllowDuplicates)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a boolean flag on the sample pool. No allocations, locks, or I/O.
**Minimal Example:** `{obj}.setAllowDuplicateSamples(true);`

**Description:**
Controls whether this expansion's sample pool allows duplicate sample file references. When set to true, the same sample files can be referenced by multiple sample maps within the expansion. When false, the pool rejects duplicate sample references.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldAllowDuplicates | Integer | no | Whether to allow duplicate sample references (true/false) | -- |

## setSampleFolder

**Signature:** `bool setSampleFolder(var newSampleFolder)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Creates a link file on disk and refreshes subdirectory state.
**Minimal Example:** `{obj}.setSampleFolder(FileSystem.getFolder(FileSystem.Samples));`

**Description:**
Redirects this expansion's Samples folder to a new location by creating a link file. The new folder must exist as a directory. If the target is the same as the current Samples folder, no link file is created and the method returns false. Returns true when a link file was successfully created and subdirectories were refreshed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newSampleFolder | ScriptObject | no | A File object pointing to the new sample directory | Must be a File object, not a string path |

**Pitfalls:**
- Requires a File object (from FileSystem.getFolder or similar). Passing a string path silently returns false because the internal dynamic_cast to ScriptFile fails.
- [BUG] Does not check whether the expansion reference is still valid before accessing the internal expansion object. If the expansion has been unloaded, this may crash instead of throwing a descriptive script error.

**Cross References:**
- `$API.Expansion.getSampleFolder$`
- `$API.Expansion.getRootFolder$`

## unloadExpansion

**Signature:** `void unloadExpansion()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to ExpansionHandler which triggers expansion removal, listener notifications, and potential voice killing.
**Minimal Example:** `{obj}.unloadExpansion();`

**Description:**
Removes this expansion from the active expansion list. After calling, the expansion will not appear in ExpansionHandler.getExpansionList() until the next application restart or re-initialisation. The current Expansion reference becomes invalid -- the internal WeakReference is nullified.

**Parameters:**
None.

**Pitfalls:**
- After calling, this Expansion reference becomes invalid. Any subsequent method call on the same reference will either throw a script error ("Expansion was deleted") or crash, depending on whether the specific method checks object validity.

**Cross References:**
- `$API.ExpansionHandler.getExpansionList$`

## writeDataFile

**Signature:** `bool writeDataFile(var relativePath, var dataToWrite)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** JSON serialization and file I/O (replaceWithText).
**Minimal Example:** `{obj}.writeDataFile("myConfig.json", {"key": "value"});`

**Description:**
Writes a JSON data file to this expansion's AdditionalSourceCode directory on the filesystem. Serializes the data to JSON format and writes it to disk using replaceWithText. Always writes to the filesystem regardless of expansion type -- does not update embedded pool data for Intermediate/Encrypted expansions. Returns true if the file was written successfully.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| relativePath | String | no | Relative path for the JSON file within the AdditionalSourceCode folder | -- |
| dataToWrite | JSON | no | The data object to serialize as JSON | -- |

**Pitfalls:**
- [BUG] Does not check whether the expansion reference is still valid before accessing the internal expansion object. If the expansion has been unloaded, this may crash instead of throwing a descriptive script error.
- [BUG] For Intermediate/Encrypted expansions, the written file exists on disk alongside the encoded data but does not modify the embedded pool. A subsequent loadDataFile call on a non-FileBased expansion will load from the pool (old data), not from the file just written.

**Cross References:**
- `$API.Expansion.loadDataFile$`
- `$API.Expansion.getDataFileList$`
