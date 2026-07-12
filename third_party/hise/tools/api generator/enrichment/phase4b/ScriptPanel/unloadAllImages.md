# unloadAllImages | UNSAFE

Removes all images previously loaded via `loadImage()` from this panel's image cache. After calling this, `isImageLoaded()` will return `false` for all previously loaded image names.

```
unloadAllImages()
```

## Pair With

- `loadImage()` - load images into the panel's cache
- `isImageLoaded()` - check if a specific image is loaded

## Source

`ScriptingApiContent.cpp` line ~4220
