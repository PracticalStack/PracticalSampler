Returns a GlobalRoutingManager reference for accessing global cables and OSC communication. Store the reference in a `const var` - each call creates a new wrapper, but all wrappers reference the same underlying singleton.

```js
const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("MyCable");
```