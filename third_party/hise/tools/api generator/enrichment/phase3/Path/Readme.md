---
keywords: Path
summary:  The Path object 
author:   Christoph Hart
modified: 31.07.2024
---

The `Path` object with which you can define a path that can be drawn to a [Panel](/ui-components/plugin-components/panel). 
You can create a new path object with [Content.createPath()](/scripting/scripting-api/content#createpath) and draw it with [Graphics.drawPath()](/scripting/scripting-api/graphics#drawpath).


```javascript
const var p = Content.createPath();

p.startNewSubPath(0.0, 0.0);
p.lineTo(0.2, 1.0);
p.lineTo(1.0, 0.2);
p.lineTo(0.7, 1.0);

const var Panel1 = Content.getComponent("Panel1");

Panel1.setPaintRoutine(function(g)
{
    g.setColour(Colours.white);
    
    var path_data = {}; // pathStrokeStyle object  
    path_data.Thickness = 3.0;
    
	g.drawPath(p, this.getLocalBounds(10), path_data);
});
```

Icon Factory:

```javascript
// 1. Create an object and populate it with icons

const var icons = {
	star: Content.createPath("102.t0F..ZBQHEgQCw1aQ2BQnQAjCwF..VDQnQAjCwFWWICQXt9pCwVynlCQXcO1CwF..ZBQn.RuCwFMWMAQXcO1CwFynpAQXt9pCwF..d.QnQAjCwVjt7AQnQAjCwF..ZBQHEgQCMVY"),
	heart: Content.createPath("162.t0l5+YBQHN3cCIlDyxBQfocRCgXF4PDXZmzPtxzODQuqfMjX9+WQDg3f2Mj++UDQLXojC4JS+PDSpk5Phga85PDlIp6P3x7KDwMpKOj5+YBQTKw0CIlFyzAQbi5xCIjBRPDlIp6PjLaCDwjZoNjX9+2ADwfkRNj++c.QHN3cCQxrMPD8tB1Phoj4SPDXZmzPDyDHDAl1IMj5+YBQHN3cCMVY")
};

// 2. Create buttons and assign the icons

const var StarButton = Content.addButton("StarButton", 0, 0);
const var HeartButton = Content.addButton("HeartButton", 0, 50);

// use the text property as key for the icon lookup
StarButton.set("text", "star");
HeartButton.set("text", "heart");

// 3. Implement a LAF that fetches the icons

const var iconLaf = Content.createLocalLookAndFeel();

iconLaf.registerFunction("drawToggleButton", function(g, obj)
{
	g.setColour(obj.textColour);
	
	// grab the path and scale it to fit into the button bounds
	var p = icons[obj.text];
	var min = Math.min(obj.area[2], obj.area[3]);
	var pb = Rectangle(obj.area).withSizeKeepingCentre(min, min);
	g.fillPath(p, pb.reduced(2));
});

StarButton.setLocalLookAndFeel(iconLaf);
HeartButton.setLocalLookAndFeel(iconLaf);
```

