Unlocker::contains(String otherString) -> Integer

Thread safety: WARNING -- String involvement, atomic ref-count operations.
Checks whether the loaded key file data contains the given substring. Can be used
to check for feature flags or product tiers embedded in key file data.

Required setup:
  const var ul = Engine.createLicenseUnlocker();
  ul.loadKeyFile(); // or auto-loaded by constructor

Pair with:
  isUnlocked -- always check unlock status before relying on contains()
  loadKeyFile -- key file must be loaded for contains() to have data to search

Anti-patterns:
  - [BUG] Returns true when unlocker reference is null, not false. An uninitialized
    unlocker appears to contain everything. Always guard with isUnlocked() first.

Source:
  ScriptExpansion.cpp  RefObject::contains()
    -> unlocker->contains(otherString) // delegates to JUCE key data search
    -> returns true if unlocker is null (permissive fallback)
