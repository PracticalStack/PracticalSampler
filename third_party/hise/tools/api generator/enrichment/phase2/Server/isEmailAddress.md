## isEmailAddress

**Examples:**

```javascript:form-validation
// Title: Validate email input before enabling a submit button
// Context: Client-side email validation for a contact or feedback form.
// isEmailAddress() runs a quick structural check (no server round-trip),
// making it suitable for real-time input validation.

const var emailInput = Content.addLabel("EmailInput", 10, 10);
emailInput.set("editable", true);
emailInput.set("text", "");
emailInput.set("saveInPreset", false);

const var submitButton = Content.addButton("SubmitButton", 10, 50);
submitButton.set("enabled", false);
submitButton.set("saveInPreset", false);

reg lastValidResult = -1;

inline function onEmailChanged(component, value)
{
    local isValid = Server.isEmailAddress(value);
    lastValidResult = isValid;

    // Enable the button only when the email looks valid
    submitButton.set("enabled", isValid);

    if (!isValid && value.length > 0)
        Console.print("Please enter a valid email address");
};

emailInput.setControlCallback(onEmailChanged);
```
```json:testMetadata:form-validation
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Server.isEmailAddress('user@example.com')", "value": 1},
    {"type": "REPL", "expression": "Server.isEmailAddress('not-an-email')", "value": 0},
    {"type": "REPL", "expression": "emailInput.setValue('test@domain.com') || emailInput.changed()", "value": false},
    {"type": "REPL", "expression": "lastValidResult", "value": 1}
  ]
}
```
