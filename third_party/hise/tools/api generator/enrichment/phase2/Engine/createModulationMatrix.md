## createModulationMatrix

**Examples:**

```javascript:modulation-matrix-setup
// Title: Setting up a modulation matrix with a Global Modulator Container
// Context: The modulation matrix requires a GlobalModulatorContainer in
// the module tree. Create the matrix by passing the container's processor
// ID. The returned object manages connections between global modulator
// sources and target parameters.

const var sm = Engine.createModulationMatrix("Global Modulator Container1");

// The matrix integrates with FloatingTile ModulationMatrix components
// for drag-and-drop UI. Set the matrixTargetId property on target
// components to enable the visual connection interface.

// Clear all modulation connections (e.g., from a reset button)
// sm.clearAllConnections("");
```
```json:testMetadata:modulation-matrix-setup
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer module in the module tree."
}
```
