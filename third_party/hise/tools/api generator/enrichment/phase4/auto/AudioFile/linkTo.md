Links this AudioFile's data slot to share the same underlying buffer as another AudioFile. After linking, both references point to the same audio data - changes through one are visible through the other.

> [!Warning:Requires same complex data type] The target must be another AudioFile. Passing a different complex data type (Table, SliderPackData) produces a type mismatch error.
