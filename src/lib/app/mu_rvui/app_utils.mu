//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: app_utils {
use commands;

VoidFunc        := (void;);
EventFunc       := (void;Event);
BindingList     := [(string, EventFunc, string)]; // (event, func, doc)
MenuStateFunc   := (int;);

\: noop (void; Event ev) {;}
\: disabledItem (int;) { DisabledMenuState; }
\: enabledItem (int;) { NeutralMenuState; }

\: makeEventFunc (EventFunc; VoidFunc f) {
    //
    //  Note we don't reject the event here.  For obscure reasons it causes
    //  problems if we do, but that means you shouldn't use this func, or '~'
    //  in cases where the event should not be consumed.
    //
    \: (void; Event ignored) { f(); };
}

\: makeCategoryEventFunc (EventFunc; string category, EventFunc f) {
    \: (void; Event ev) {
        if (!commands.isActionCategoryEnabled(category))
            sendInternalEvent("action-blocked");
        else
            f(ev);
    };
}

operator: ~ (EventFunc; VoidFunc f)
{
    makeEventFunc(f);
}

operator: && (MenuStateFunc; MenuStateFunc Fa, MenuStateFunc Fb)
{
    \: (int;)
    {
        let a = Fa(),
            b = Fb();
        
        return if a == b then a
                  else if a == DisabledMenuState then a
                  else if b == DisabledMenuState then b
                  else if a == CheckedMenuState then a
                  else if b == CheckedMenuState then b
                  else a;
    };
}

\: bind (void; string mode, string table, string event, VoidFunc F, string doc="")
{
    bind(mode, table, event, makeEventFunc(F), doc);
}

\: bind (void; string mode, string table, string event, string category, VoidFunc F, string doc="")
{
    bind(mode, table, event, makeCategoryEventFunc(category, makeEventFunc(F)), doc);
}

\: bind (void; string ev, VoidFunc F, string doc="") 
{ 
    bind("default", "global", ev, F, doc); 
}

\: bind (void; string ev, string category, VoidFunc F, string doc="") 
{ 
    bind("default", "global", ev, makeCategoryEventFunc(category, makeEventFunc(F)), doc); 
}

\: bind (void; string ev, EventFunc F, string doc="") 
{ 
    bind("default", "global", ev, F, doc); 
}

\: bind (void; string ev, string category, EventFunc F, string doc="") 
{ 
    bind("default", "global", ev, makeCategoryEventFunc(category, F), doc); 
}

\: unbind (void; string ev)
{
    unbind("default", "global", ev);
}

\: bind (void; string mode, string table, BindingList bindings)
{
    for_each (b; bindings) bind(mode, table, b._0, b._1);
}

\: bindRegex (void; string mode, string table, string eventRegex, VoidFunc F)
{
    bindRegex(mode, table, eventRegex, makeEventFunc(F));
}

\: bindRegex (void; string mode, string table, string eventRegex, string category, VoidFunc F)
{
    bindRegex(mode, table, eventRegex, makeCategoryEventFunc(category, makeEventFunc(F)));
}

\: bindRegex (void; string eventRegex, VoidFunc F)
{
    bindRegex("default", "global", eventRegex, makeEventFunc(F));
}

\: bindRegex (void; string eventRegex, string category, VoidFunc F)
{
    bindRegex("default", "global", eventRegex, makeCategoryEventFunc(category, makeEventFunc(F)));
}

\: reverse (string[]; string[] inarray)
{
    string[] outarray;
    [string] slist = nil;
    for_each (x; inarray) slist = x : slist;
    for_each (x; slist) outarray.push_back(x);
    outarray;
}

\: call_with_gc_api (void; (void; Event) F, int api, Event event)
{
    runtime.gc.push_api(api);
    try { F(event); } catch (...) { runtime.gc.pop_api(); throw; }
    runtime.gc.pop_api();
}

//
//  Standard action category validator - checks if category is enabled
//
\: _withCategoryValidator (int; string categoryName, (int;)stateFunc) 
{
    if (commands.isActionCategoryEnabled(categoryName))
        return stateFunc();
    return DisabledMenuState;
}

//
//  Default action blocked callback
//
\: _defaultActionBlockedFunc(void;)
{
    sendInternalEvent("action-blocked");
}

//
//  Internal function for binding actions and creating menu items
//

\: _eventNameToKeyShortcut (string; string eventName)
{
    if (eventName == "")
    {
        return "";
    }
    
    // Remove "key-down--" prefix
    let key = eventName;

/*    
    if (key.startsWith("key-down--"))
    {
        key = key.substring(10);  // Remove "key-down--"
    }
    // Convert modifiers
    key = key.replace("control--", "Ctrl+");
    key = key.replace("alt--", "Alt+");
    key = key.replace("shift--", "Shift+");
    
    // Convert special keys
    key = key.replace("left", "Left");
    key = key.replace("right", "Right");
    key = key.replace("up", "Up");
    key = key.replace("down", "Down");
    key = key.replace("home", "Home");
    key = key.replace("end", "End");
    key = key.replace("pageup", "Page Up");
    key = key.replace("pagedown", "Page Down");
    key = key.replace("space", "Space");
    key = key.replace("tab", "Tab");
    key = key.replace("enter", "Enter");
    key = key.replace("return", "Return");
    key = key.replace("escape", "Escape");
    key = key.replace("backspace", "Backspace");
    key = key.replace("delete", "Delete");
    
    // Convert single characters to uppercase
    if (key.length() == 1)
    {
        key = key.toUpper();
    }
*/    
    
    return key;
}


//
//  Global convenience functions
//

\: menuSeparator (MenuItem;)
{
    return MenuItem {"_", nil};
}

\: subMenu (MenuItem; string label, MenuItem[] subMenu)
{
    return MenuItem {label, nil, "", enabledItem, subMenu};
}

\: menuText (MenuItem; string text)
{
    return MenuItem {text, nil, "", disabledItem, nil};
}
 
\: menuItem (MenuItem; string menuText, string eventPattern, string category, EventFunc func, 
                MenuStateFunc stateFunc, string description = "")
{
    if (category == nil) category = "";
    if (eventPattern == nil) eventPattern = "";
    if (description == nil) description = "";
/*
    // Create composite validator that combines category validation with state validation
    let compositeStateFunc = \: (int;) {
        if (!commands.isActionCategoryEnabled(category))
            return DisabledMenuState;
        else
            return stateFunc();
    };

    let compositeFunc = \: (void; Event ev) {
        if (compositeStateFunc() == DisabledMenuState)
            sendInternalEvent("action-blocked");
        else
            func(ev);
    };
*/
    // Perform the bind action
    /*
    if (eventPattern != "")
    {
        commands.bind("default", "global", eventPattern, compositeFunc, description);
    }
    */
    
    // Generate accelerator from event pattern
    let menuKeyText = "";
//    if (eventPattern != nil && eventPattern.startsWith("key-down--"))
//    {
//        menuKeyText = _eventNameToKeyShortcut(eventPattern);
//    }
    
    // Return MenuItem object
//    return MenuItem {menuText, compositeFunc, menuKeyText, compositeStateFunc, nil};
    return MenuItem {menuText, func, menuKeyText, stateFunc, nil};
}

}
