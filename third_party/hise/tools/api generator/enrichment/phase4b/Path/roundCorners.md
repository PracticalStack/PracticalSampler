Path::roundCorners(Number radius) -> undefined

Thread safety: UNSAFE -- creates an entirely new JUCE Path via createPathWithRoundedCorners, which allocates heap memory
Replaces the current path with a version where all sharp corners have been
smoothed with curves of the given radius. This is destructive and non-reversible
-- the original sharp-corner geometry cannot be recovered. A radius of 0.0
produces no visible change.

Dispatch/mechanics:
  p = p.createPathWithRoundedCorners(radius)
  Entire internal path replaced. Applies uniformly to all corners in all sub-paths.

Source:
  ScriptingGraphics.cpp  PathObject::roundCorners()
    -> p = p.createPathWithRoundedCorners(radius)
