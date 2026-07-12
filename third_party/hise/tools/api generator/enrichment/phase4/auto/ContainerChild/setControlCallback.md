Registers a callback that fires when this component's value changes. The callback receives one argument (the new value). Inside the callback, `this` refers to the ContainerChild. The callback deduplicates - setting the same value twice only fires it once.

> [!Warning:Callback takes one argument, not two] Unlike ScriptComponent control callbacks which receive `(component, value)`, this callback receives only the value. This is a common source of confusion when migrating from standard components.
