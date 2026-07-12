Math::to0To1(Number value, Object rangeObj) -> Double

Thread safety: SAFE
Converts a value within a range to normalised 0.0-1.0. Inverse of Math.from0To1.
Accepts the same three range object conventions and scriptnode fix objects.
Dispatch/mechanics:
  getRange(rangeObj) parses JSON into InvertableParameterRange
    -> checks for scriptnode (MinValue/MaxValue/SkewFactor/StepSize),
       UI Component (min/max/middlePosition/stepSize),
       or MIDI Automation (Start/End/Skew/Interval) conventions
    -> middlePosition variants call setSkewForCentre() internally
    -> range.convertTo0to1(value, true) with inversion support
Anti-patterns:
  - Confusing middlePosition (UI Component convention) with SkewFactor
    (scriptnode convention) -- these are different values for the same curve
Pair with:
  from0To1 -- inverse conversion (normalised to real value)
  skew -- compute skew factor from a midpoint value
Source:
  JavascriptEngineMathObject.cpp:328-407  MathClass::getRange() static helper
    -> InvertableParameterRange::convertTo0to1()
  NodeProperty.h:40-99  InvertableParameterRange wraps juce::NormalisableRange
