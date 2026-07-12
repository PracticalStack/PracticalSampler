Encrypts an intermediate `.hxi` file into a credential-encrypted `.hxp` file using the credentials previously set via `setCredentials()`. Pass a `File` object pointing to the `.hxi` file. Returns `true` on success.

The simplest install flow is to let the user browse for a downloaded `.hxi` file and encode it immediately:

```javascript
const var eh = Engine.createExpansionHandler();

FileSystem.browse(FileSystem.Documents, false, "*.hxi", function(hxiFile)
{
    eh.encodeWithCredentials(hxiFile);
});
```

For more complex workflows, combine this with the `Server` download API to fetch the `.hxi` programmatically. After encoding, `refreshExpansions()` is called automatically.

> [!Warning:Credentials must be set first] Call `setCredentials()` before this method. Without valid credentials, the encryption produces an invalid `.hxp` file.
