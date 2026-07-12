WavetableController::setEnableResynthesisCache(ScriptObject cacheDirectory, Integer clearCache) -> undefined

Thread safety: UNSAFE -- file system operations (directory access, optional deletion of .tmp files)
Enables caching of resynthesis results in the specified directory. Cache files
are named by hash(filename) + hash(options JSON) so entries auto-invalidate when
either changes. Pass true for clearCache to delete all existing .tmp files first.

Pair with:
  resynthesise -- cached results are checked before running resynthesis
  setResynthesisOptions -- changing options invalidates cache entries

Source:
  ScriptingApiObjects.cpp  setEnableResynthesisCache()
    -> wt->setResynthesisCache(cacheDir, clearCache)
    -> cache stored as .tmp files: hash(filename) + "_" + hash(JSON(options)) + ".tmp"
