Returns a Modulator handle for the internal modulation chain at the given index. The returned chain is itself a Modulator, so you can set its attributes, bypass it, add child modulators to it, or poll its combined output level with `getCurrentLevel` for UI visualisation.

> [!Warning:Chain indices vary by module type] Chain indices are specific to the module type. Gain is typically 0 and pitch typically 1, but effects and other modules may use different layouts. Check the HISE module documentation for the specific module.
