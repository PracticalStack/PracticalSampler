# control.cable_table - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1315` (cable_table class)
**Base class:** `data::base`, `parameter_node_base<ParameterClass>`, `no_processing`
**Classification:** control_source

## Signal Path

The cable_table node uses a 512-float lookup table to reshape a normalised control signal. The input value (0..1) is used as a normalised index into the table with linear interpolation. The looked-up value is sent to the output.

Value parameter (0..1) -> normalised index into 512-float table -> linear interpolation -> table value -> output (normalised)

## Gap Answers

### table-lookup-interpolation: Does cable_table use the standard table lookup pattern?

Yes, confirmed exactly. Lines 1349-1350 define:
```
using TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE>;
using InterpolatorType = index::lerp<index::normalised<double, TableClampType>>;
```
This is `index::lerp<index::normalised<double, index::clamped<512>>>` -- normalised input [0,1], clamped bounds at 512, linear interpolation between adjacent table entries. The lookup in `setValue()` (line 1339): `InterpolatorType ip(input); auto tv = tableData[ip];`.

The output range depends on the table contents. Default table values are 0..1 (the table UI constrains values to 0..1), so the output is normally 0..1.

### table-display-value: Does cable_table call setDisplayedValue()?

Yes. Line 1345: `this->externalData.setDisplayedValue(input)`. The input value (0..1) is sent to the table UI to show the current lookup position as a vertical cursor.

### table-empty-behaviour: What happens when the table data is empty?

When tableData is empty, the `if (!tableData.isEmpty())` check (line 1336) prevents any processing. No output is sent, and the display is not updated. The node effectively does nothing until a table is connected.

## Parameters

- **Value** (single parameter via SN_ADD_SET_VALUE): Normalised input 0..1, default 0.0. This is the table lookup position. NOT marked as unscaled (cable_table does not inherit from `no_mod_normalisation`).

## Complex Data

- **Table** (1 table): 512-float lookup table. Connected via `setExternalData()` which calls `base::setExternalData()` and stores a block reference to channel 0 of the table data (line 1329).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The node stores `lastValue` (line 1353) which is updated on each `setValue()` call. This is used to re-apply the table lookup when the table data changes (the `setExternalData()` callback calls `setValue(lastValue)` at line 1331 to refresh the output with new table data).
