# Chapter 8 - Modes and Widgets

The user interface layer can augment the display and event handling in a number of different ways. For display, at the lowest level it's possible to intercept the render event in which case you override all drawing. Similarily for event handling you can bind functions in the global event table possibly overwriting existing bindings and thus replace their functions.At a higher level, both display and event handling can be done via Modes and Widgets. A Mode is a class which manages an event table independent of the global event table and a collection of functions which are bound in that table. In addition the mode can have a render function which is automatically called at the right time to augment existing rendering instead of replacing it. The UI has code which manages modes so that they may be loaded externally only when needed and automatically turned on and off.Modes are further classified as being minor or major. The only difference between them is that a major mode will always get precedence over any minor mode when processing events and there can be only a single major mode active at a time. There can be many minor modes active at once. Most extensions are created by creating a minor mode. RV currently has a single basic major mode.

![13_lease_event_prop.png](../../images/rv-reference-manual-13-rv-cx-lease-event-prop-12.png)

Figure 8.1:Event Propagation. Red and Green modes process the event. On the left the Red mode rejects the event allowing it to continue. On the right Red mode does not reject the event stopping the propagation.By using a mode to implement a new feature or replace or augment an existing feature in RV you can keep your extensions separate from the portion of the UI that ships with RV. In other words, you never need to touch the shipped code and your code will remain isolated.A further refinement of a mode is a widget. Widgets are minor modes which operate in a constrained region of the screen. When the pointer is in the region, the widget will receive events. When the pointer is outside the region it will not. Like a regular mode, a widget has a render function which can draw anywhere on the screen, but usually is constrainted to its input region. For example, the image info box is a widget as is the color inspector.Multiple modes and widgets may be active at the same time. At this time Widgets can only be programmed using Mu.

### 8.1 Outline of a Mode


In order to create a new mode you need to create a module for it and derive your mode class from the MinorMode class in the rvtypes module. The basic outline which we'll put in a file called new_mode.mu looks like this:

```
 use rvtypes;

module: new_mode {

class: NewMode : MinorMode
{
    method: NewMode (NewMode;)
    {
        init ("new-mode",
              [ global bindings ... ],
              [ local bindings ... ],
              Menu(...) );
    }
}

\: createMode (Mode;)
{
    return NewMode();
}

} // end of new_mode module 
```
The function createMode() is used by the mode manager to create your mode without knowing anything about it. It should be declared in the scope of the module (not your class) and simply create your mode object and initialize it if that's necessary.When creating a mode it's necessary to call the init() function from within your constructor method. This function takes at least three arguments and as many as six. Chapter [10](rv-reference-manual-chapter-ten.md#chapter-10-a-simple-package) goes into detail about the structure in more detail. It's declared like this in rvtypes.mu:

```
 method: init (void;
              string name,
              BindingList globalBindings,
              BindingList overrideBindings,
              Menu menu = nil,
              string sortKey = nil,
              int ordering = 0) 
```
The name of the mode is meant to be human readable.The “bindings” arguments supply event bindings for this mode. The bindings are only active when the mode is active and take precedence over any “global” bindings (bindings not associated with any mode). In your event function you can call the “reject” method on an event which will cause rv to pass it on to bindings “underneath” yours. This technique allows you to augment an existing binding instead of replacing it. The separation of the bindings into overrideBindings and globalBindings is due to backwards compatibility requirements, and is no longer meaningful.The menu argument allows you to pass in a menu structure which is merged into the main menu bar. This makes it possible to add new menus and menu items to the existing menus.Finally the sortKey and ordering arguments allow fine control over the order in which event bindings are applied when multiple modes are active. First the ordering value is checked (default is 0 for all modes), then the sortKey (default is the mode name).Again, see chapter [10](rv-reference-manual-chapter-ten.md#chapter-10-a-simple-package) for more detailed information.

### 8.2 Outline of a Widget


A Widget looks just like a MinorMode declaration except you will derive from Widget instead of MinorMode and the base class init() function is simpler. In addition, you'll need to have a render() method (which is optional for regular modes).

```
 use rvtypes;

module: new_widget {

class: NewWidget : Widget
{
    method: NewWidget (NewWidget;)
    {
        init ("new-widget",
              [ local bindings ... ] );
    }

    method: render (void; Event event)
    {
        ...
        updateBounds(min_point, max_point);
        ...
    }
}

\: createMode (Mode;)
{
    return NewWidget();
}

} // end of new_widget module 
```
In the outline above, the function updateBounds() is called in the render() method. updateBounds() informs the UI about the bounding box of your widget. This function must be called by the widget at some point. If your widget can be interactively or procedurally moved, you will probably want to may want to call it in your render() function as shown (it does not hurt to call it often). The min_point and max_point arguments are Vec2 types.