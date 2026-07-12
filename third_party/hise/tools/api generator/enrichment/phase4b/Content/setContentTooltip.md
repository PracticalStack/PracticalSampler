Content::setContentTooltip(String tooltipText) -> undefined

Thread safety: SAFE -- simple string assignment to a member variable.
Sets the tooltip text for the entire content area. This tooltip appears when hovering
over the background of the interface where no component is present.

Pair with:
  getCurrentTooltip -- read the tooltip of the component under the mouse

Source:
  ScriptingApiContent.cpp  Content::setContentTooltip()
    -> simple string assignment to member variable
