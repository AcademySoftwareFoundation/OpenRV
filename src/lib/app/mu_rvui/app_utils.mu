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


\: checkAndBlockEventCategory(bool; string category)
{
    if (!commands.isEventCategoryEnabled(category))
    {
        sendInternalEvent("category-event-blocked", category);
        return false;
    }
    return true;
}

\: makeCategoryEventFunc (EventFunc; string category, EventFunc f) {
    \: (void; Event ev) {
        if (!commands.isEventCategoryEnabled(category))
            sendInternalEvent("category-event-blocked", category);
        else
            f(ev);
    };
}

operator: ~ (EventFunc; VoidFunc f)
{
    makeEventFunc(f);
}

\: bind (void; string mode, string table, string event, VoidFunc F, string doc="")
{
    bind(mode, table, event, makeEventFunc(F), doc);
}

\: bind (void; string mode, string table, string event, string category, VoidFunc F, string doc="")
{
    bind(mode, table, event, makeCategoryEventFunc(category, makeEventFunc(F)), doc);
}

\: bind (void; string mode, string table, string event, string category, EventFunc F, string doc="")
{
    bind(mode, table, event, makeCategoryEventFunc(category, F), doc);
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
//  Standard event category validator - checks if category is enabled
//

\: menu_debug(void; string prefix, string text)
{
    if (false)
        print("MENU DEBUG: (%s) %s\n" % (prefix, text));
}

//
//  Internal function for binding actions and creating menu items
//

\: _eventNameToKeyShortcut (string; string eventName)
{

    let key = eventName;

    // check if it starts with "key-down--"
    if (string.contains(key, "key-down--") != 0)
        return "";

    key = string.replace(key, "key-down--", "");
    // start with space since we will add spaces later for the modifiers.
    key = key.replace(" ", "space");
    
    // Convert double-dash format to space-separated format for RvDocument::buildShortcutString
    key = key.replace("control--", "control ");
    key = key.replace("alt--", "alt ");
    key = key.replace("shift--", "shift ");
    key = key.replace("meta--", "meta ");
    key = key.replace("command--", "command ");
    
    // Convert arrow keys to match buildShortcutString expectations
    // these in the event pattern already match what is expected in the menu key shortcut
    // key = key.replace("left", "left");
    // key = key.replace("right", "right");
    // key = key.replace("up", "up");
    // key = key.replace("down", "down");

    menu_debug("key shortcut", "[\"%s\" -> \"%s\"]" % (eventName, key));
    
    return key;
}


//
//  Global convenience functions
//


\: newMenu (MenuItem[]; MenuItem[] items)
{
    menu_debug("newMenu", "");
    return items;
}

\: subMenu (MenuItem; string label, MenuItem[] subItems)
{
    menu_debug("subMenu", "\"%s\"" % label);
    let sm = MenuItem {label, nil, "", enabledItem, subItems};
    return sm;
}

\: menuSeparator (MenuItem;)
{
    menu_debug("menuSeparator", "");
    let ms = MenuItem {"_", nil};
    return ms;
}

\: menuText (MenuItem; string text)
{
    menu_debug("menuText", "\"%s\"" % text);
    let mt = MenuItem {text, nil, "", disabledItem, nil};
    return mt;
}
 
\: menuItem (MenuItem; string menuText, string eventPattern, string category, EventFunc func, 
                MenuStateFunc stateFunc)
{
    menu_debug("menuItem", "\"%s\"" % menuText);

    // Create composite validator that combines category validation with state validation
    let compositeStateFunc = \: (int;) {
        if (!commands.isEventCategoryEnabled(category))
            return DisabledMenuState;
        else
            return stateFunc();
    };

    // create composite function that checks if the category is enabled and then calls the function
    let compositeFunc = \: (void; Event ev) {
        if (!commands.isEventCategoryEnabled(category))
            sendInternalEvent("category-event-blocked", category);
        else
            func(ev);
    };

    // Perform the bind action
    if (eventPattern != "")
    {
        string description = menuText;

        // strip description of all leading spaces
        while (description.contains(" ") == 0)
        {
            description = description.substr(1, -1);
        }

        commands.bind("default", "global", eventPattern, compositeFunc, description);
    }
    
    // Generate accelerator from event pattern
    let menuKeyText = _eventNameToKeyShortcut(eventPattern);

    let mi = MenuItem {menuText, compositeFunc, menuKeyText, compositeStateFunc, nil};
    return mi;
}

}
