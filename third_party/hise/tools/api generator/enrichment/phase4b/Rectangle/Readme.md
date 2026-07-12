Rectangle (object)
Obtain via: Rectangle(x, y, w, h), Rectangle([x, y, w, h]), Rectangle(otherRect)

Mutable rectangle utility for layout slicing, resizing, containment checks, and
geometric operations. Wraps JUCE's Rectangle<double> with scripting-friendly
property access (rect.x, rect.width) and full interoperability with [x,y,w,h] arrays.

Constructors:
  Rectangle()                    -- empty rectangle
  Rectangle([x, y, w, h])       -- from array
  Rectangle(width, height)      -- at origin [0, 0]
  Rectangle(x, y, w, h)         -- full specification
  Rectangle([x1, y1], [x2, y2]) -- from two corner points
  Rectangle(otherRect)          -- copy from a preexisting Rectangle object

Complexity tiers:
  1. Basic layout slicing: removeFromTop, removeFromLeft, removeFromBottom,
     removeFromRight, reduced, withSizeKeepingCentre. Handles the vast majority
     of real-world layout work.
  2. Non-mutating transformations: translated, withX, withY, withWidth, withHeight,
     withCentre, withSize. Copy a region at a different position or size without
     modifying the source.
  3. Geometry queries and advanced layout: contains, intersects, constrainedWithin,
     withAspectRatioLike, getIntersection, getUnion. Hit testing, icon fitting,
     and constraint-based layout.

API return types and wrapping:
  By default, getLocalBounds(), obj.area, and similar APIs return plain [x,y,w,h]
  arrays. Wrap the result in Rectangle() at the top of paint routines and LAF
  callbacks -- this is the recommended pattern because it works regardless of
  project settings:
    var rect = Rectangle(this.getLocalBounds(0));
    var area = Rectangle(obj.area);
  When the source already returns a Rectangle (with HISE_USE_SCRIPT_RECTANGLE_OBJECT=1),
  the wrapping constructor creates a lightweight copy -- negligible overhead.
  Alternative for new projects: set HISE_USE_SCRIPT_RECTANGLE_OBJECT=1 in
  preprocessor definitions to make all APIs return Rectangle objects directly.

Practical defaults:
  - Use Rectangle(this.getLocalBounds(0)) or Rectangle(obj.area) as the starting
    point for all layout slicing inside paint routines and LAF callbacks.
  - Use withSizeKeepingCentre to center icons and indicators inside allocated areas
    instead of manual centering arithmetic.
  - Use reduced(amount) for uniform padding, reduced(x, y) for asymmetric padding.
    Negative values expand the rectangle (useful for hover/focus outlines).
  - Store pre-computed Rectangle objects in panel.data during setup rather than
    recomputing them each paint cycle.

Common mistakes:
  - Treating removeFrom* as non-mutating -- these methods mutate the source
    rectangle AND return the removed strip. The source shrinks after each call.
  - Using manual [x,y,w,h] array arithmetic instead of the built-in Rectangle
    methods. The Rectangle class provides all layout slicing natively with method
    chaining.
  - Calling removeFrom* on obj.area when it is an array -- mutates the original.
    Wrap in Rectangle(obj.area) first to create an independent copy.

Example:
  // Recommended pattern: wrap API return values in Rectangle()
  Panel1.setPaintRoutine(function(g)
  {
      var rect = Rectangle(this.getLocalBounds(0));
      g.fillAll(0xFF222222);

      var header = rect.removeFromTop(50);
      g.setColour(0xFFCCCCCC);
      g.setFont("Oxygen Bold", 18.0);
      g.drawAlignedText("Title", header.reduced(10, 0), "left");

      var sidebar = rect.removeFromLeft(100);
      g.setColour(0xFF333333);
      g.fillRect(sidebar);

      // rect now contains the remaining center area
      g.setColour(0xFF111111);
      g.fillRoundedRectangle(rect.reduced(4), 4.0);
  });

Methods (34):
  constrainedWithin       contains                expanded
  getIntersection         getUnion                intersects
  isEmpty                 reduced                 removeFromBottom
  removeFromLeft          removeFromRight         removeFromTop
  scaled                  setCentre               setPosition
  setSize                 toArray                 translated
  withAspectRatioLike     withBottom              withBottomY
  withCentre              withHeight              withLeft
  withRight               withSize                withSizeKeepingCentre
  withTrimmedBottom       withTrimmedLeft         withTrimmedRight
  withTrimmedTop          withWidth               withX
  withY
