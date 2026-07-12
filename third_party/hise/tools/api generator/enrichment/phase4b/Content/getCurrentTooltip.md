Content::getCurrentTooltip() -> String

Thread safety: SAFE -- queries JUCE Desktop for mouse source and TooltipClient interface, no allocations.
Returns the tooltip text of the component currently under the mouse cursor. Returns
an empty string if no tooltip-capable component is under the mouse or if touch input
is active.

Pair with:
  setContentTooltip -- set the tooltip for the content background area

Source:
  ScriptingApiContent.cpp  Content::getCurrentTooltip()
    -> Desktop mouse source -> TooltipClient interface query
