Sets the displayed image by loading it from the project's image pool. The file name should use the `{PROJECT_FOLDER}` prefix to reference images in the project's `Images/` folder, or an expansion wildcard reference for expansion-specific artwork. Pass an empty string to clear the image. After loading, any active blend mode is recomputed automatically.

> [!Warning:forceUseRealFile has no effect] The `forceUseRealFile` parameter has no effect - images are always loaded through the pool regardless of this value. Use `set("fileName", path)` as an equivalent alternative.
