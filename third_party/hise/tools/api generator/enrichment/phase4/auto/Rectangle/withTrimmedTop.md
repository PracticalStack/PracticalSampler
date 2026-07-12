Returns a new Rectangle with the given amount removed from the top edge, without modifying the original.

![withTrimmedTop](geometry_withTrimmedTop.svg)

Unlike `removeFromTop()`, this does not mutate the source and does not return the removed strip - it returns the remaining area only.