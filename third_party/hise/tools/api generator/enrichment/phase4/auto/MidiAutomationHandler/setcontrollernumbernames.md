Customises the display names used in the right-click MIDI automation popup. The `ccName` parameter sets the popup section header (replacing the default "MIDI CC" label), and the `nameArray` provides per-CC display names indexed by CC number. CC numbers beyond the array length fall back to the default "CC#N" format.

Pair this with `setControllerNumbersInPopup()` so the popup shows only the relevant controllers with readable labels.
