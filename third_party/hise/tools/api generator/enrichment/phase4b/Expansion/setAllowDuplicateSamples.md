Expansion::setAllowDuplicateSamples(var shouldAllowDuplicates) -> undefined

Thread safety: SAFE -- sets a boolean flag on the sample pool, no allocations, locks, or I/O
Controls whether this expansion's sample pool allows duplicate sample file references.
When true, the same sample files can be referenced by multiple sample maps within the expansion.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Source:
  ScriptExpansion.cpp  ScriptExpansionReference::setAllowDuplicateSamples()
    -> exp->pool->getSamplePool()->setAllowDuplicateSamples(shouldAllowDuplicates)
