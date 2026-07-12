Enables or disables GPU-to-CPU buffer readback after each shader render. When enabled, every frame's shader output is copied from the GPU into a CPU-side image buffer, which is required for `Content.createScreenshot()` to capture shader output.

Only enable this when screenshot capture is needed. The per-frame `glReadPixels` call adds measurable overhead that is unnecessary during normal rendering.
