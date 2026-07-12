Returns the JSON layer description of the currently loaded model. Only works with models created via `build` or `loadTensorFlowModel`.

> [!Warning:Returns empty object for NAM and compiled models] NAM models, compiled models, and the default empty model do not support JSON export. The method returns an empty object silently with no error.
