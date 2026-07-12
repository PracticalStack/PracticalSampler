Validates an RSA-encrypted expiration timestamp and, on success, unlocks the plugin for the encoded duration. The `encodedTimeString` must be a hex string starting with `"0x"` containing an RSA-encrypted ISO 8601 date. On success, returns the number of days remaining as an integer. On failure, returns `false`. If the input format is invalid or the Unlocker is unavailable, returns an error string.

```js
var result = ul.checkExpirationData(encodedHexFromServer);

if (typeof result == "number")
    Console.print("Valid for " + result + " days");
else if (result == false)
    Console.print("Expiration check failed");
else
    Console.print("Error: " + result);
```

> [!Warning:Return type varies - check with typeof] The return value is polymorphic: an integer on success, `false` on RSA failure, or an error string on format problems. Error strings are truthy, so testing `if (result)` alone is not sufficient - use `typeof result == "number"` to confirm success.