## assertIsObjectOrArray

**Examples:**

```javascript:validating-module-references-before
// Title: Validating module references before method calls
// Context: When working with data that should be objects or arrays,
// assertIsObjectOrArray catches primitive values that would cause
// errors in subsequent property access or method calls.

const var moduleList = [
    {"id": "Osc1", "type": "Oscillator"},
    {"id": "Filter1", "type": "Filter"}
];

inline function processModule(module)
{
    Console.assertIsObjectOrArray(module);
    Console.print("Processing: " + module.id);
}

for (m in moduleList)
    processModule(m);

// Passing a string instead of an object triggers the assertion
processModule("not_an_object");
```
```json:testMetadata:validating-module-references-before
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Processing: Osc1", "Processing: Filter1"]},
    {"type": "expect-error", "errorMessage": "Assertion failure"}
  ]
}
```


```javascript:function-parameter-validation
// Title: Function parameter validation
// Context: Utility functions that expect a module reference or
// structured data object use assertIsObjectOrArray as a type guard.

const var testModule = {"name": "TestModule", "level": 0.75};

inline function checkModuleType(module)
{
    Console.assertIsObjectOrArray(module);
    Console.print("Module validated: " + module.name);
}

checkModuleType(testModule);
```
```json:testMetadata:function-parameter-validation
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": [
      "Module validated: TestModule"
    ]
  }
}
```

