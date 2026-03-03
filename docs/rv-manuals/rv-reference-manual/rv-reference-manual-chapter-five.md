# Chapter 5 - Event Handling

Aside from rendering, the most important function of the UI is to handle events. An event can be triggered by any of the following:

* The mouse pointer moved or a button on the mouse was pressed
* A key on the keyboard was pressed or released
* The window needs to be re-rendered
* A file being watched was changed
* The user became active or inactive
* A supported device (like the apple remote control) did something
* An internal event like a new source or session being created has occurred

Each specific event has a name may also have extra data associated with it in the form of an event object. To see the name of an event (at least for keyboard and mouse pointer events) you can select the Help → Describe... which will let you interactively see the event name as you hit keys or move the mouse. You can also use Help → Describe Key.. to see what a specific key is bound to by pressing it.Table [5.1](#event-prefixes-for-basic-device-events) shows the basic event type prefixes.

| Event Prefix | Description |
| --- | --- |
| key-down | Key is being pressed on the keyboard |
| key-up | Key is being released on the keyboard |
| pointer | The mouse moved, button was pressed, or the pointer entered (or left) the window |
| dragdrop | Something was dragged onto the window (file icon, etc) |
| render | The window needs updating |
| user | The user's state changed (active or inactive, etc) |
| remote | A network event |

Table 5.1: Event Prefixes for Basic Device Events <a id="event-prefixes-for-basic-device-events"></a>

When an event is generated in RV, the application will look for a matching event name in its bindings. The bindings are tables of functions which are assigned to certain event names. The tables form a stack which can be pushed and popped. Once a matching binding is found, RV will execute the function.When receiving an event, all of the relevant information is in the Event object. This object has a number of methods which return information depending on the kind of event.

| Method | Events | Description |
| --- | --- | --- |
| pointer (Vec2;) | pointer-\* dragdrop-\* | Returns the location of the pointer relative to the view. |
| relativePointer (Vec2;) | pointer-\* dragdrop-\* | Returns the location of the pointer relative to the current widget or view if there is none. |
| reference (Vec2;) | pointer-\* dragdrop-\* | Returns the location of initial button mouse down during dragging. |
| domain (Vec2;) | pointer-\* render-\* dragdrop-\* | Returns the size of the view. |
| subDomain (Vec2;) | pointer-\* render-\* dragdrop-\* | Returns the size of the current widget if there is one. relativePointer() is positioned in the subDomain(). |
| buttons (int;) | pointer-\* dragdrop-\* | Returns an int or'd from the symbols: Button1, Button2, and Button3. |
| modifiers (int;) | pointer-\* key-\* dragdrop-\* | Returns an int or'd from the symbols: None, Shift, Control, Alt, Meta, Super, CapLock, NumLock, ScrollLock. |
| key (int;) | key-\* | Returns the “keysym” value for the key as an int |
| name (string;) | any | Returns the name of the event |
| contents (string;) | internal eventsdragdrop-\* | Returns the string content of the event if it has any. This is normally the case with internal events like new-source, new-session, etc. Pointer, key, and other device events do not have a contents() and will throw if it's called on them. Drag and drop events return the data associated with them. Some render events have contents() indicating the type of render occurring. |
| contentsArray (string[];) | internal events | Same as contents(), but in the case of some internal events ancillary information may be present which can be used to avoid calling additional commands. |
| sender (string;) | any | Returns the name of the sender |
| contentType (int;) | dragdrop-\* | Returns an int describing the contents() of a drag and drop event. One of: UnknownObject, BadObject, FileObject, URLObject, TextObject. |
| timeStamp (float;) | any | Returns a float value in seconds indicating when the event occurred |
| reject (void;) | any | Calling this function will cause the event to be send to the next binding found in the event table stack. Not calling this function stops the propagation of the event. |
| setReturnContents (void; string) | internal events | Events which have a contents may also have return content. This is used by the remote network events which can have a response. |

Table 5.2:Event Object Methods. Python methods have the same names and return the same value types.

### 5.1 Binding an Event

In Mu (or Python) you can bind an event using any of the bind() functions. The most basic version of bind() takes the name of the event and a function to call when the event occurs as arguments. The function argument (which is called when the event occurs) should take an Event object as an argument and return nothing (void). Here's a function that prints hello in the console every time the \`\`j'' key is pressed:

> **Note:** If this is the first time you've seen this syntax, it's defining a Mu function. The first two characters \\: indicate a function definition follows. The name comes next. The arguments and return type are contained in the parenthesis. The first identifier is the return type followed by a semicolon, followed by an argument list.

E.g, \: add (int; int a, int b) { return a + b; }

```
\: my_event_function (void; Event event)
{
    print("Hello!\n");
}

bind("key-down--j", my_event_function); 
```

or in Python:

```
 def my_event_function (event):
    print ("Hello!")

bind("default", "global", "key-down--j", my_event_function); 
```

There are more complicated bind() functions to address binding functions in specific event tables (the Python example above is using the most general of these). Currently RV's user interface has one default global event table an couple of other tables which implement the parameter edit mode and help modes.Many events provide additional information in the event object. Our example above doesn't even use the event object, but we can change it to print out the key that was pressed by changing the function like so:

```
 \: my_event_function (void; Event event)
{
    let c = char(event.key());
    print("Key pressed = %c\n" % c);
} 
```

or in Python:

```
 def my_event_function (event):
    c = event.key()
    print ("Key pressed = %s\n" % c) 
```

In this case, the Event object's key() function is being called to retrieve the key pressed. To use the return value as a key it must be cast to a char. In Mu, the char type holds a single unicode character. In Python, a string is unicode. See the section on the Event class to find out how to retrieve information from it. At this point we have not talked about *where* you would bind an event; that will be addressed in the customization sections.

### 5.2 Keyboard Events

There are two keyboard events: key-down and key-up. Normally the key-down events are bound to functions. The key-up events are necessary only in special cases.The specific form for key down events is key-down– *something* where *something* uniquely identifies both the key pressed and any modifiers that were active at the time.So if the \`\`a'' key was pressed the event would be called: key-down–a. If the control key were held down while hitting the \`\`a'' key the event would be called key-down–control–a.There are five modifiers that may appear in the event name: alt, caplock, control, meta, numlock, scrolllock, and shift in that order. The shift modifier is a bit different than the others. If a key is pressed with the shift modifier down and it would result in a different character being generated, then the shift modifier will not appear in the event and instead the result key will. This may sound complicated but these examples should explain it:For control + shift + A the event name would be key-down–control–A. For the \`\`\*'' key (shift + 8 on American keyboards) the event would be key-down–\*. Notice that the shift modifier does not appear in any of these. However, if you hold down shift and hit enter on most keyboards you will get key-down–shift–enter since there is no character associated with that key sequence.Some keys may have a special name (like enter above). These will typically be spelled out. For example pressing the \`\`home'' key on most keyboards will result in the event key-down–home. The only way to make sure you have the correct event name for keys is to start RV and use the Help → Describe... facility to see the true name. Sometimes keyboards will label a key and produce an unexpected event. There will be some keyboards which will not produce an event all for some keys or will produce a unicode character sequence (which you can see via the help mechanism).

### 5.3 Pointer (Mouse) Events

The mouse (called pointer from here on) can produce events when it is moved, one of its buttons is pressed, an attached scroll wheel is rotated, or the pointer enters or leaves the window.The basic pointer events are move, enter, leave, wheelup, wheeldown, push, drag, and release. All but enter and leave will also indicate any keyboard modifiers that are being pressed along with any buttons on the mouse that are being held down. The buttons are numbered 1 through 5. For example if you hold down the left mouse button and movie the mouse the events generated are:

```
pointer-1--push
pointer-1--drag
pointer-1--drag
...
pointer-1-release 
```

Pointer events involving buttons and modifiers always come in there parts: push, drag and release. So for example if you press the left mouse, move the mouse, press the shift key, move the mouse, release everything you get:

```
pointer-1--push
pointer-1--drag
pointer-1--drag
...
pointer-1-release
pointer-1--shift--push
pointer-1--shift--drag
pointer-1--shift--drag
...
pointer-1--shift--release 
```

Notice how the first group without the shift is released before starting the second group with the shift even though you never released the mouse button. For any combination of buttons and modifiers, there will be a push-drag-release sequence that is cleanly terminated.It is also possible to hold multiple mouse buttons and modifiers down at the same time. When multiple buttons are held (for example, button 1 and 2) they are simply both included (like the modifiers) so for buttons 1 and 2 the name would be pointer-1-2–push to start the sequence.The mouse wheel behaves more like a button: when the wheel moves you get only a wheelup or wheeldown event indicating which direction the wheel was rotated. The buttons and modifiers will be applied to the event name if they are held down. Usually the motion of the wheel on a mouse will not be smooth and the event will be emitted whenever the wheel \`\`clicks''. However, this is completely a function of the hardware so you may need to experiment with any particular mouse.There are three more pointer events that can be generated. When the mouse moves with no modifiers or buttons held down it will generate the event pointer–move. When the pointer enters the view pointer–enter is generated and when it leaves pointer–leave. Something to keep in mind: when the pointer leaves the view and the device is no longer in focus on the RV window, any modifiers or buttons the user presses will not be known to RV and will not generate events. When the pointer returns to the view it may have modifiers that became active when out-of-focus. Since RV cannot know about these modifiers and track them in a consistent manner (at least on X Windows) RV will assume they do not exist.Pointer events have additional information associated with them like the coordinates of the pointer or where a push was made. These will be discussed later.

### 5.4 The Render Event

The UI will get a render event whenever it needs to be updated. When handling the render event, a GL context is set up and you can call any GL function to draw to the screen. The event supplies additional information about the view so you can set up a projection.At the time the render event occurs, RV has already rendered whatever images need to be displayed. The UI is then called in order to add additional visual objects like an on-screen widget or annotation.Here's a render function that draws a red polygon in the middle of the view right on top of your image.Listing 5.1:Example Render Function

```
 \: my_render (void; Event event)
{
    let domain = event.domain(),
        w      = domain.x,
        h      = domain.y,
        margin = 100;

    use gl;
    use glu;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Big red polygon
    glColor(Color(1,0,0,1));
    glBegin(GL_POLYGON);
    glVertex(margin, margin);
    glVertex(w-margin, margin);
    glVertex(w-margin, h-margin);
    glVertex(margin, h-margin);
    glEnd();
} 
```

Note that for Python, you will need to use the PyOpenGL module or bind the symbols in the gl Mu module manually in order to draw in the render event.The UI code already has a function called render() bound the render event; so this function basically turns off existing UI rendering.

### 5.5 Remote Networking Events

RV's networking generates a number of events indicating the status of the network. In addition, once a connection has been established, the UI may generate sent to remote programs, or remote programs may send events to RV. These are typically uniquely named events which are specific to the application that is generating and receiving them.For example the sync mechanism generates a number of events which are all named remote-sync-something.

### 5.6 Internal Events

Some events will originate from RV itself. These include things like new-source or new-session which include information about what changed. The most useful of these is new-source which can be used to manage color and other image settings between the time a file is loaded and the time it is first displayed. (See Color Management Section). Other internal events are functional, but are placeholders which will become useful with future features.The current internal events are listed in table [5.3](#internal-events) .

| Event | Event.(data/contents) | Ancillary Data (contentsArray) | Description |
| --- | --- | --- | --- |
| render | | | Main view render |
| pre-render | | | Before rendering |
| post-render | | | After rendering |
| per-render-event-processing | | | Qt Event processing between renders (a “safe” time to edit the graph) |
| layout | | | Main view layout used to handle view margin changes |
| new-source | nodename;;RVSource;;filename | | **DEPRECATED** A new source node was added (or media was reset) |
| source-group-complete | group nodename;;action_type | | A new or modified source group is complete |
| source-modified | nodename;;RVSource;;filename | | An existing source was changed |
| media-relocated | nodename;;oldmedia;;newmedia | | A movie, image sequence, audio file was swapped out |
| source-media-set | nodename;;tag | | |
| before-session-read | filename | | Session file is about to be read |
| after-session-read | filename | | Session file was read |
| before-session-write | filename | | Session file is about to be written |
| after-session-write | filename | | Session file was just written |
| before-session-write-copy | filename | | A copy of the session is about to be written |
| after-session-write-copy | filename | | A copy of a session was just written |
| before-session-deletion | | | The session is about to be deleted |
| before-graph-view-change | nodename | | The current view node is about to change. |
| after-graph-view-change | nodename | | The current view node changed. |
| new-node | nodename | | A new view node was created. |
| graph-new-node | nodename | nodename protocol version groupname | A new node of any kind was created. |
| set-current-annotate-mode-node | nodename | | Set the Paint node to use for the _currentNode in the annotate mode package. Calling this event with an empty string will reset its behaviour. |
| annotate-mode-activated | | | The annotate mode package is activated |
| before-progressive-loading | | | Loading will start |
| after-progressive-loading | | | Loading is complete (sent immediately if no files will be loaded) |
| graph-layer-change | | | **DEPRECATED** use after-graph-view-change |
| frame-changed | | | The current frame changed |
| fps-changed | | | Playback FPS changed |
| play-start | | | Playback started |
| play-stop | | | Playback stopped |
| incoming-source-path | infilename;;tag | | A file was selected by the user for loading. |
| missing-image | | | An image could not be loaded for rendering |
| cache-mode-changed | buffer or region or off | | Caching mode changed |
| view-size-changed | | | The viewing area size changed |
| new-in-point | frame | | The in point changed |
| new-out-point | frame | | The out point changed |
| before-source-delete | nodename | | Source node will be deleted |
| after-source-delete | nodename | | Source node was deleted |
| before-node-delete | nodename | | View node will be deleted |
| after-node-delete | nodename | | View node was deleted |
| after-clear-session | | | The session was just cleared |
| after-preferences-write | | | Preferences file was written by the Preferences GUI |
| state-initialized | | | Mu/Python init files read |
| session-initialized | | | All modes toggled, command-line processed, etc. |
| realtime-play-mode | | | Playback mode changed to realtime |
| play-all-frames-mode | | | Playback mode changed to play-all-frames |
| before-play-start | | | Play mode will start |
| mark-frame | frame | | Frame was marked |
| unmark-frame | frame | | Frame was unmarked |
| update-hold-button | | | Update the checked state of the hold button |
| update-ghost-button | | | Update the checked state of the ghost button |
| pixel-block | Event.data() | | A block of pixels was received from a remote connection |
| graph-state-change | | | A property in the image processing graph changed |
| graph-node-inputs-changed | nodename | | Inputs of a top-level node added/removed/re-ordered |
| range-changed | | | The time range changed |
| narrowed-range-changed | | | The narrowed time range changed |
| margins-changed | left right top bottom | | View margins changed |
| output-video-device-changed | outputVideoModuleName/outputVideoDeviceName | | The output video device changed |
| view-resized | old-w new-w | old-h new-h | |
| preferences-show | | | Pref dialog will be shown |
| preferences-hide | | | Pref dialog was hidden |
| read-cdl-complete | cdl_filename;;cdl_nodename | | CDL file has been loaded |
| read-lut-complete | lut_filename;;lut_nodename | | LUT file has been loaded |
| remote-eval | code | | Request to evaluate external Mu code |
| remote-pyeval | code | | Request to evaluate external Python code |
| remote-pyexec | code | | Request to execute external Python code |
| remote-network-start | | | Remote networking started |
| remote-network-stop | | | Remote networking stopped |
| remote-connection-start | contact-name | | A new remote connection has been made |
| remote-connection-stop | contact-name | | A remote connection has died |
| remote-contact-error | contact-name | | A remote connection error occurred while being established |

Table 5.3:Internal Events <a id="internal-events"></a>

#### 5.6.1 File Changed Event

It is possible to watch a file from the UI. If the watched file changes in any way (modified, deleted, moved, etc) a file-changed event will be generated. The event object will contain the name of the watched file that changed. A function bound to file-changed might look something like this:

```
 \: my_file_changed (void; Event event)
{
    let file = event.contents();
    print("%s changed on disk\n" % file);
} 
```

In order to have a file-changed event generated, you must first have called the command function watchFile().

#### 5.6.2 Incoming Source Path Event

This event is sent when the user has selected a file or sequence to load from the UI or command line. The event contains the name of the file or sequence. A function bound to this event can change the file or sequence that RV actually loads by setting the return contents of the event. For example, you can cause RV to check and see if a single file is part of a larger sequence and if so load the whole sequence like so:

```
 \: load_whole_sequence (void; Event event)
{
    let file        = event.contents(),
        (seq,frame) = sequenceOfFile(event.contents());

    if (seq != "") event.setReturnContent(seq);
}

bind("incoming-source-path", load_whole_sequence); 
```

or in Python:

```
 def load_whole_sequence (event):

    file = event.contents();
    (seq,frame) = rv.commands.sequenceOfFile(event.contents());

    if seq != "":
         event.setReturnContent(seq);


bind("default", "global", "incoming-source-path", load_whole_sequence, "Doc string"); 
```

#### 5.6.3 Missing Images

Sometimes an image is not available on disk when RV tries to read. This is often the case when looking at an image sequence while a render or composite is ongoing. By default, RV will find a nearby frame to represent the missing frame if possible. The missing-image event will be sent once for each image which was expected but not found. The function bound to this event can render information on on the screen indicating that the original image was missing. The default binding display a message in the feedback area.The missing-image event contains the domain in which rendering can occur (the window width and height) as well as a string of the form \`\`frame;source'' which can be obtained by calling the contents() function on the event object.The default binding looks like this:

```
 \: missingImage (void; Event event)
{
    let contents = event.contents(),
        parts = contents.split(";"),
        media = io.path.basename(sourceMedia(parts[1])._0);

    displayFeedback("MISSING: frame %s of %s"
                     % (parts[0], media), 1, drawXGlyph);
}

bind("missing-image", missingImage); 
```

#### 5.6.4 Drag and Drop Filtering

Sometimes dragging in a directory into RV, the directory might contain files that RV cannot load (e.g. sidecar files), the directory-filter allows you to filter out certain file types.

The Filter should return "1" if you want the file, "0" if you dont.

A python example of using this is:

```
def filter_directory (event):
  file = event.contents();
  if file.endswith(".info"):
    event.setReturnContent("0");
    return
  event.setReturnContent("1");

commands.bind("default", "global", "directory-filter", filter_directory, "Doc string");
```
