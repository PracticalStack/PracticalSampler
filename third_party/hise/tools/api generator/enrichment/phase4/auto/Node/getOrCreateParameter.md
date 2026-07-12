This is the only way to obtain a [Parameter]($API.Parameter$) reference for controlling scriptnode parameters from script. It works on all node types - both leaf nodes and containers.

When called with a name or index that matches an existing parameter, it returns that parameter. When called with a name that does not exist (or a JSON object defining a new parameter), it creates a new macro parameter - but only on container nodes. Attempting to create a new parameter on a leaf node produces a script error.

```javascript
// Get an existing parameter on any node type (leaf or container)
const var gain = gainNode.getOrCreateParameter("Gain");
gain.setValueAsync(0.5);

// Create a new macro parameter (container nodes only)
const var macro = container.getOrCreateParameter("MyMacro");
```

Pass a string name, integer index, or a JSON object with ID and range properties (MinValue, MaxValue, SkewFactor, StepSize, mode) to define a new parameter.
