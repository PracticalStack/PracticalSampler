Loads an image from the project's `Images` folder and stores it under the given alias. The alias is then used to reference the image in paint functions via `g.drawImage()`. Supports expansion pack references with the `{EXP::Name}` prefix. If an image with the same alias already exists and the file reference differs, the image is silently replaced.

> [!Warning:Missing files produce no error] If the image file is not found, only a console warning is printed - no script error is thrown. Use `isImageLoaded()` after loading to verify the image was found before referencing it in draw functions.
