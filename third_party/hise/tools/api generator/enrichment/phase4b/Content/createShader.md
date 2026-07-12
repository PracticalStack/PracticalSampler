Content::createShader(String fileName) -> ScriptObject

Thread safety: UNSAFE -- heap-allocates a ScriptShader object and registers it as a screenshot listener.
Creates a ScriptShader for GLSL-based rendering in ScriptPanels. If fileName is not
empty, loads the fragment shader from the project's shader folder. Pass an empty string
to create without loading a file immediately. Automatically registered as screenshot listener.

Required setup:
  const var shader = Content.createShader("myEffect");

Dispatch/mechanics:
  new ScriptShader -> registered as screenshotListener
  If fileName not empty: calls setFragmentShader(fileName)
  HISE_SUPPORT_GLSL_LINE_NUMBERS: optional debug line numbers

Pair with:
  ScriptShader.setFragmentShader -- load/change shader file after creation
  ScriptPanel.setShader -- assign shader to a panel for rendering

Source:
  ScriptingApiContent.cpp:8747  Content::createShader()
    -> new ScriptShader (heap allocation)
    -> registers as screenshot listener
    -> optional setFragmentShader(fileName) if non-empty
