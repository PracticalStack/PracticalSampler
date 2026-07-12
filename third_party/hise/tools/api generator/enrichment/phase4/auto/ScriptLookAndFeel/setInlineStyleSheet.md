Parses a string as CSS code and applies it to all components registered to this LookAndFeel object. If the CSS contains syntax errors, a script error is thrown immediately. Passing an empty string disables CSS mode.

For complex stylesheets, prefer `setStyleSheet()` with an external `.css` file. The external file provides proper CSS syntax highlighting, autocomplete, and fast iteration via F5 reparsing - none of which are available with inline strings.
