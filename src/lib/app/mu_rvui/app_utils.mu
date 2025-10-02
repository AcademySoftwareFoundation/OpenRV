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

\: menu_debug(void; string prefix, string text)
{
    if (false)
        print("MENU DEBUG: (%s) %s\n" % (prefix, text));
}

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

\: menuSeparator (MenuItem;)
{
    menu_debug("menuSeparator", "");
    let ms = MenuItem {"_", nil};
    return ms;
}

\: subMenu (MenuItem; string label, MenuItem[] subMenu)
{
    menu_debug("subMenu", "\"%s\"" % label);
    let sm = MenuItem {label, nil, "", enabledItem, subMenu};
    return sm;
}   

\: menuText (MenuItem; string text)
{
    menu_debug("menuText", "\"%s\"" % text);
    let mt = MenuItem {text, nil, "", disabledItem, nil};
    return mt;
}
 
\: menuItem (MenuItem; string menuText, string eventPattern, string category, EventFunc func, 
                MenuStateFunc stateFunc, string description = "")
{
    menu_debug("menuItem", "\"%s\"" % menuText);

    // Create composite validator that combines category validation with state validation
    let compositeStateFunc = \: (int;) {
        if (!commands.isActionCategoryEnabled(category))
            return DisabledMenuState;
        else
            return stateFunc();
    };

    // create composite function that checks if the category is enabled and then calls the function
    let compositeFunc = \: (void; Event ev) {
        if (compositeStateFunc() == DisabledMenuState)
            sendInternalEvent("action-blocked");
        else
            func(ev);
    };

    // Perform the bind action
    if (eventPattern != "")
    {
        commands.bind("default", "global", eventPattern, compositeFunc, description);
    }
    
    // Generate accelerator from event pattern
    let menuKeyText = _eventNameToKeyShortcut(eventPattern);

    let mi = MenuItem {menuText, func, menuKeyText, stateFunc, nil};
    return mi;
}

}
