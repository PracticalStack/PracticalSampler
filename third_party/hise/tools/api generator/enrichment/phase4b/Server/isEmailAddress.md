Server::isEmailAddress(String email) -> Integer

Thread safety: SAFE -- delegates to JUCE's URL::isProbablyAnEmailAddress, a simple pattern check with no allocations or I/O.
Checks whether the given string looks like a valid email address. Basic structural check (presence of @, domain part). Not RFC-compliant -- catches obvious formatting errors but may accept some invalid addresses. Does not perform DNS lookup or SMTP verification.
Source:
  ScriptingApi.cpp  Server::isEmailAddress()
    -> URL::isProbablyAnEmailAddress(email)
