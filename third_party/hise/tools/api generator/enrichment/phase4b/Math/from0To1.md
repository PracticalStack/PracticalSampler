Math::from0To1(Number value, Object rangeObj) -> Double

Thread safety: SAFE
Converts a normalised value (0.0-1.0) to a value within the range defined by
rangeObj. Supports three JSON naming conventions and scriptnode fix objects.
Dispatch/mechanics:
  getRange(rangeObj) parses JSON into InvertableParameterRange
    -> checks for scriptnode (MinValue/MaxValue/SkewFactor/StepSize),
       UI Component (min/max/middlePosition/stepSize),
       or MIDI Automation (Start/End/Skew/Interval) conventions
    -> middlePosition variants call setSkewForCentre() internally
    -> range.convertFrom0to1(value, true) with inversion support
Anti-patterns:
  - Using without SkewFactor or middlePosition for frequency ranges -- linear
    mapping is almost never correct for perceptual parameters
  - Confusing middlePosition (UI Component convention) with SkewFactor
    (scriptnode convention) -- these are different values for the same curve
Pair with:
  to0To1 -- inverse conversion (real value to normalised)
  skew -- compute skew factor from a midpoint value
Source:
  JavascriptEngineMathObject.cpp:328-407  MathClass::getRange() static helper
    -> InvertableParameterRange::convertFrom0to1()
  NodeProperty.h:40-99  InvertableParameterRange wraps juce::NormalisableRange
