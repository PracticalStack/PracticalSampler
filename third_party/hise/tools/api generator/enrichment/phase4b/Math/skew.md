Math::skew(Number start, Number end, Number midPoint) -> Double

Thread safety: SAFE
Returns the skew factor for a range where midPoint maps to 0.5 in normalised
space. The returned value can be used as SkewFactor in range objects passed to
Math.from0To1 and Math.to0To1.
Dispatch/mechanics:
  Creates temporary NormalisableRange(start, end)
    -> setSkewForCentre(midPoint)
    -> returns rng.skew
Pair with:
  from0To1 -- uses the skew factor for range conversion
  to0To1 -- uses the skew factor for inverse range conversion
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::skew()
    -> NormalisableRange::setSkewForCentre(midPoint) -> returns .skew
