This function can be used by automated tools (AI Agents etc). in order to manually call defined callbacks of UI components / other objects. It allows synchronous execution of any callback function with a predetermined set of arguments that can be used to test the functionality of the script without relying on user interaction.

```javascript
// Create a panel
const var Panel1 = Content.addPanel("Panel", 0, 0);

// Allow the mouse callback to receive clicks & hover events.
// Note: The test callback function will also check that this
// property is set to the minimal level to process the incoming 
// event object, so if you eg. set it to "Clicks Only" and then
// call testCallback with an object that defines the hover flag
// it will fail.
// It will also check all provided JSON properties against the
// list of valid properties, so if the LLM starts hallucinating
// weird event properties, this will be catched in the development
// cycle.
Panel1.set("allowCallbacks", "Clicks & Hover");

// Now we just define a simple mouse callback that stores the
// hover state in the data object.
Panel1.setMouseCallback(function(event)
{
	this.data.down = event.hover;
	Console.print("this.data.down is: " + this.data.down);
	this.repaint();
});

// Now we call it with the given JSON object as event object.
// Note that this will be executed synchronously without waiting
// for the UI thread or anything, so don't use this in any other
// scenario than for testing purposes
Console.testCallback(Panel1, "setMouseCallback", {
	x: 100,
	y: 0,
	hover: true
});

// Since the function was executed immediately, this state flag
// should have been set properly
Console.assertTrue(Panel1.data.down);

// Call it again to disable the down property flag
Console.testCallback(Panel1, "setMouseCallback", {
	x: 100,
	y: 0,
	hover: false
});

// Verify that the second callback test reset the flag.
Console.assertTrue(Panel1.data.down == false);


/*  The Console output looks a bit jagged, but if you run that 
	script with the HISE CLI tool using this syntax:

	HISE compile_script -p:TestScript.js

	You'll get a neatly formatted JSON response that can be used
	by the AI Agent in the development cycle to confirm the functionality

	This is how the output will look:
*/

const var EXAMPLE_RESPONSE = {
  "file": "ABSOLUTE_PATH_TO_FILE",
  "status": "ok",     // ["ok", "error"] indicating the script execution state
  "compileTimeMs": 1, // time in milliseconds
  "screenshot": null, // if you use the command line flag -screenshot:width,height
  				      // this will create a PNG screenshot of the interface that can
  				      // be used for analysis / image recognition tooling
  "callback_tests": [
    {
      "uuid": "#1",	  // a consecutive id
      "id": "Panel",  // The object ID where this callback is called upon
      "callback": "setMouseCallback", // the callback ID
      "args": {		  // the exact argument list (or single argument)
        "x": 100,
        "y": 0,
        "hover": 1
      },
      "status": "ok", // ["ok", "error"] indicating whether there was a runtime error
      				  //                 in the callback execution
      "output": [
        "this.data.down is: 1" // Any console output during the callback execution is
        					   // piped into this object so it can be analysed better.
      ],
      "errors": [] // If there was an error, this will contain the callstack
    },
    {
      "uuid": "#2",
      "id": "Panel",
      "callback": "setMouseCallback",
      "args": {
        "x": 100,
        "y": 0,
        "hover": 0
      },
      "status": "ok",
      "output": [
        "this.data.down is: 0"
      ],
      "errors": []
    }
  ],
  "output": [ // The console output outside of callbacks.
    "test #1: Panel.setMouseCallback", // whenever a callback is tested, this will be
    "test #2: Panel.setMouseCallback"  // inserted to the output so you can see the 
    								   // chronological order
  ],
  "errors": [] // If there was an error outside the callback execution
};
```




