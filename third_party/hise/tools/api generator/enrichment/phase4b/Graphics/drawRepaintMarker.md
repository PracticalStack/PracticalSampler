Graphics::drawRepaintMarker(String label) -> undefined

Thread safety: UNSAFE -- allocates a new draw action and a StringBuilder
Debug utility: fills the entire component with a random semi-transparent colour
(30% opacity, random hue) to visualize repaint frequency. Each call produces a
different colour -- rapid changes indicate excessive repainting. The label is used
for Perfetto profiling traces when PERFETTO is defined.

Anti-patterns:
  - This overwrites all existing drawn content. Place at the start of a paint
    routine to see the indicator behind other content, or use temporarily only.

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawRepaintMarker()
    -> g.fillAll(Colour::fromHSL(random, 0.33, 0.5, 0.3))
    -> PERFETTO counter track if profiling enabled
