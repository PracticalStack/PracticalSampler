## setImageProvider

**Examples:**

```javascript:path-icon-provider
// Title: Embed vector icons in markdown text
// Context: A tooltip or help panel uses inline icons to illustrate
// UI controls. Path objects are registered as image providers,
// then referenced in markdown text via ![alt](/url) syntax.

const var tooltipMd = Content.createMarkdownRenderer();

// Create path icons (in practice, load from saved path data)
const var muteIcon = Content.createPath();
muteIcon.addRoundedRectangle([0, 0, 1, 1], 0.1);

const var soloIcon = Content.createPath();
soloIcon.addEllipse([0, 0, 1, 1]);

// Register path-based image providers
const var iconProviders = [
    {
        "Type": "Path",
        "URL": "/mute",
        "Colour": 0xFF333333,
        "Data": muteIcon
    },
    {
        "Type": "Path",
        "URL": "/solo",
        "Colour": 0xFF333333,
        "Data": soloIcon
    }
];

tooltipMd.setImageProvider(iconProviders);

// Reference icons in markdown using the URL strings
tooltipMd.setText("### Controls\n| -- | ------- |\n| ![](/solo:22px) | Solo the track |\n| ![](/mute:22px) | Mute the track |");
tooltipMd.setTextBounds([0, 0, 300, 400]);
```

```json:testMetadata:path-icon-provider
{
  "testable": false,
  "skipReason": "Path data and image rendering require visual verification"
}
```

```javascript:pool-image-provider
// Title: Embed pool images in markdown text
// Context: An image entry references an image from the HISE image pool
// using the standard pool reference format.

const var md = Content.createMarkdownRenderer();

const var imageProviders = [
    {
        "URL": "/logo",
        "Reference": "{PROJECT_FOLDER}logo.png"
    }
];

md.setImageProvider(imageProviders);
md.setText("# Welcome\n![logo](/logo)");
md.setTextBounds([0, 0, 400, 300]);
```

```json:testMetadata:pool-image-provider
{
  "testable": false,
  "skipReason": "Requires image file in project pool"
}
```
