File::getHash() -> String

Thread safety: UNSAFE -- reads entire file from disk for hashing (I/O plus CPU-intensive SHA-256).
Computes the SHA-256 hash of the file and returns it as a lowercase hex string.

Anti-patterns:
  - Reads the entire file into memory for hashing. For very large files, this
    consumes significant memory and blocks the calling thread.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getHash()
    -> SHA256(f).toHexString()
