## assertNoString

**Examples:**

```javascript:catching-accidental-string-values
// Title: Catching accidental string values in data arrays
// Example: Validating data structure fields aren't corrupted to strings
// Context: When deserializing data, fields that should be objects or numbers
// can be corrupted to strings. assertNoString catches this data corruption.

var mixerData = [
    {Name: {path: "icon1.svg"}, Channel: 1},
    {Name: "corrupted_string", Channel: 2}  // Should be object, not string
];

// Check second entry - Name field is corrupted to string
Console.assertNoString(mixerData[1].Name); // Assertion fires
```
```json:testMetadata:catching-accidental-string-values
{
  "testable": true,
  "verifyScript": {
    "type": "expect-error",
    "errorMessage": "Assertion failure"
  }
}
```


**Pitfalls:**
- The error message on failure is the string value itself (e.g., `"Assertion failure: hello"`). This can be confusing if the string content resembles a different error. Use `assertWithMessage` if you need a clearer failure message for string-type checks.
