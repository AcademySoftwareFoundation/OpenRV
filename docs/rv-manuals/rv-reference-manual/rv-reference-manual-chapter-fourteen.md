# Chapter 14 - Webkit JavaScript Integration

RV can communicate with JavaScript running in a Qt WebEngine widget. This makes it possible to serve custom RV-aware web pages which can interact with a running RV. JavaScript running in the web page can execute arbitrary Mu script strings as well as receive events from RV.You can experiment with this using the example webview package included with RV.

### 14.1 Executing Mu or Python from JavaScript

RV exports a JavaScript object called rvsession to the Javascript runtime environment. Two of the functions in that namespace are evaluate() and pyevaluate(). By calling evaluate() or pyevaluate() or pyexec() you can execute arbitrary Mu or Python code in the running RV to control it. If the executed code returns a value, the value will be converted to a string and returned by the (py)evaluate() functions. Note that pyevaluate() triggers a python eval which takes an expression and returns a value. pyexec() on the other hand takes an arbitrary block of code and triggers a python exec call.As an example, here is some html which demonstates creating a link in a web page which causes RV to start playing when pressed:

```
<script type="text/javascript">
function play () { rvsession.evaluate("play()"); }
</script>

<p><a href="javascript:play()">Play</a></p>  
```

If inlining the Mu or Python code in each call back becomes onerous you can upload function definitions and even whole classes all in one evaluate call and then call the defined functions later. For complex applications this may be the most sane way to handle call back evaluation.

### 14.2 Getting Event Call Backs in JavaScript


RV generates events which can be converted into call backs in JavaScript. This differs slightly from how events are handled in Mu and Python.

| Signal | Events |
| --- | --- |
| eventString | Any internal RV event and events generated by the command sendInternalEvent() command in Mu or Python |
| eventKey | Any key- event (e.g. key-down–a) |
| eventPointer | Any pointer- event (e.g. pointer-1–push) or tablet event (e.g. stylus-pen–push) |
| eventDragDrop | Any dragdrop- event |

Table 14.1:JavaScript Signals Produced by Events

The rvsession object contains signal objects which you can connect by supplying a call back function. In addition you need to supply the name of one or more events as a regular expression which will be matched against incoming events. For example:

```
function callback_string (name, contents, sender)
{
    var x = name + " " + contents + " " + sender;
    rvsession.evaluate("print(\"callback_string " + x + "\\n\");");
}

rvsession.eventString.connect(callback_string);
rvsession.bindToRegex("source-group-complete"); 
```
connects the function callback_string() to the eventString signal object and binds to the source-group-complete RV event. For each event the proper signal object type must be used. For example pointer events are not handled by eventString but by the eventPointer signal. There are four signals available: eventString, eventKey, eventPointer, and eventDragDrop. See tables describing which events generate which signals and what the signal call back arguments should be.In the above example, any time media is loaded into RV the callback_string() function will be called. Note that there is a single callback for each type of event. In particular if you want to handle both the “new-source” and the “frame-changed” events, your eventString handler must handle both (it can distinguish between them using the “name” parameter passed to the handler. To bind the handler to both events you can call “bindToRegex” multiple times, or specify both events in a regular expression:

```
 rvsession.bindToRegex("source-group-complete|frame-changed"); 
```

The format of this regular expression is specified [on the qt-project website](http://qt-project.org/doc/qt-4.8/qregexp.html) .

| Argument | Description |
| --- | --- |
| eventName | The name of the RV event. For example “ **source-group-complete** ” |
| contents | A string containing the event contents if it has any |
| senderName | Name of the sender if it has one |

Table 14.2:eventString Signal Arguments

| Argument | Description |
| --- | --- |
| eventName | The name of the RV event. For example “ **source-group-complete** ” |
| key | An integer representing the key symbol |
| modifiers | An integer the low order five bits of which indicate the keyboard modifier state |

Table 14.3:eventKey Signal Arguments

| Argument | Description |
| --- | --- |
| eventName | The name of the RV event. For example “ **source-group-complete** ” |
| x | The horizontal position of the mouse as an integer |
| y | The vertical position of the mouse as an integer |
| w | The width of the event domain as an integer |
| h | The height of the event domain as an integer |
| startX | The starting horizontal position of a mouse down event |
| startY | The starting vertical position of a mouse down event |
| buttonStates | An integer the lower order five bits of which indicate the mouse button states |
| activationTime | The relative time at which button activation occurred or 0 for regular pointer events |

Table 14.4:eventPointer Signal Arguments

| Argument | Description |
| --- | --- |
| eventName | The name of the RV event. For example “ **source-group-complete** ” |
| x | The horizontal position of the mouse as an integer |
| y | The vertical position of the mouse as an integer |
| w | The width of the event domain as an integer |
| h | The height of the event domain as an integer |
| startX | The starting horizontal position of a mouse down event |
| startY | The starting vertical position of a mouse down event |
| buttonStates | An integer the lower order five bits of which indicate the mouse button states |
| dragDropType | A string the value of which will be one of “enter”, “leave”, “move”, or “release” |
| contentType | A string the value of which will be one of “file”, “url”, or “text” |
| stringContent | The contents of the drag and drop event as a string |

Table 14.5:eventDragDrop Signal Arguments

### 14.3 Using the webview Example Package


This package creates one or more docked Qt WebEngine instances, configurable from the command line as described below. JavaScript code running in the webviews can execute arbitrary Mu code in RV by calling the rvsession.evaluate() function. This package is intended as an example.These command-line options should be passed to RV after the -flags option. The webview options below are shown with their default values, and all of them can apply to any of four webviews in the Left, Right, Top, and Bottom dock locations.

```
 shell> rv -flags ModeManagerPreload=webview 
```
The above forces the load of the webview package which will display an example web page. Additional arguments can be supplied to load specific web pages into additional panes. While this will just show the sample html/javascript file that comes with the package in a webview docked on the right. To see what's happening in this example, bring up the Session Manager so you can see the Sources appearing and disappearing, or switch to the defaultLayout view. Note that you can play while reconfiguring the session with the javascript checkboxes.The following additional arguments can be passed via the -flags mechanism. In the below, **POS** should be replaced by one of Left, Right, Bottom, or Top.

ModeManagerPreload=webview

Force loading of the webview package. The package should not be loaded by default but does need to be installed. This causes rv to treat the package as if it were loaded by the user.

webviewUrlPOS=URL

A webview pane will be created at POS and the URL will be loaded into it. It can be something from a web server or a file:// URL. If you force the package to load, but do not specify any URL, you'll get a single webview in the Right dock lockation rendering the sample html/javascript page that ships with the package. Note that the string "EQUALS" will be replaced by an "=" character in the URL.

webviewTitlePOS=string

Set the title of the webview pane to string.

webviewShowTitlePOS=true or false

A value of true will show and false will remove the title bar from the webview pane.

webviewShowProgressPOS=true or false

Show a progress bar while loading for the web pane.

webviewSizePOS=integer

Set the width (for right and left panes) or height (for top and bottom panes) of the web pane.

An example using all of the above:

```
 shell> rv -flags ModeManagerPreload=webview \
      webviewUrlRight=file:///foo.html \
      webviewShowTitleRight=false \
      webviewShowProgressRight=false \
      webviewSizeRight=200 \
      webviewUrlBottom=file:///bar.html \
      webviewShowTitleBottom=false \
      webviewShowProgressBottom=false \
      webviewSizeBottom=300 
```