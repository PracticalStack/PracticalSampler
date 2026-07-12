## getFilterModeList

**Examples:**

```javascript:filter-mode-combo-box
// Title: Populating a filter selector with mode constants
// Context: Rather than hardcoding filter mode integers, store the
// filter mode list object and index into it from a combo box callback.
// This makes the code readable and robust against future mode additions.
// --- setup ---
const var builder = Synth.createBuilder();
builder.create(builder.Effects.PolyphonicFilter, "Filter1", 0, builder.ChainIndexes.FX);
builder.flush();
Content.addComboBox("FilterSelector", 0, 0);
// --- end setup ---

const var filterModes = Engine.getFilterModeList();

// Map combo box items to specific filter modes
const var availableFilters = [
    filterModes.StateVariableLP,
    filterModes.StateVariableHP,
    filterModes.Allpass
];

const var myFilter = Synth.getEffect("Filter1");

inline function onFilterSelectorControl(component, value)
{
    if (value)
    {
        local mode = availableFilters[parseInt(value - 1)];
        myFilter.setAttribute(myFilter.Mode, mode);
    }
};

Content.getComponent("FilterSelector").setControlCallback(onFilterSelectorControl);
```
```json:testMetadata:filter-mode-combo-box
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof filterModes", "value": "object"},
    {"type": "REPL", "expression": "filterModes.StateVariableLP >= 0", "value": true},
    {"type": "REPL", "expression": "availableFilters.length", "value": 3}
  ]
}
```
