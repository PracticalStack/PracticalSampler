Sends an HTTP POST request to the given sub-URL with the parameters sent in the request body rather than the URL. Use POST for any request that transmits sensitive data such as credentials, form data, or device identifiers.

> [!Warning:Handle status zero as no response] Always handle `status == 0` separately from other error codes. Status 0 means no response was received (timeout or no internet), while codes like 403 or 500 are actual server responses that may contain a useful error message in the response body.
