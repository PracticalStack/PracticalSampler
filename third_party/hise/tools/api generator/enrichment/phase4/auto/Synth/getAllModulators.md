Returns an array of `ScriptModulator` handles for all modulators in the entire module tree whose IDs match the given wildcard pattern. The pattern supports `*` as a glob wildcard. Use `".*"` to match all modulators across the entire project.

> [!Warning:Searches entire module tree globally] Unlike `getModulator`, this method searches the entire module tree, not just the parent synth's subtree. A pattern like `"LFO*"` returns LFOs from all synths in the project.
