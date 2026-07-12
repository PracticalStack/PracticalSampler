Path::scaleToFit(Number x, Number y, Number width, Number height, Integer preserveProportions) -> undefined

Thread safety: SAFE
Transforms the path geometry to fit within the specified rectangle. When
preserveProportions is true, the path is uniformly scaled maintaining aspect
ratio. When false, stretches independently on X and Y to fill the entire area.
This permanently modifies all coordinates.

Pair with:
  getBounds -- to inspect path bounds before/after scaling
  setBounds -- DO NOT confuse with scaleToFit (see anti-patterns)

Anti-patterns:
  - Do NOT use setBounds to resize a path -- it only expands the reported
    bounding box without transforming geometry. Use scaleToFit instead.

Source:
  ScriptingGraphics.cpp  PathObject::scaleToFit()
    -> p.applyTransform(p.getTransformToScaleToFit(x, y, w, h, preserveProportions))
