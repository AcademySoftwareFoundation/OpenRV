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

\: makeEventFunc (EventFunc; VoidFunc f) {
    //
    //  Note we don't reject the event here.  For obscure reasons it causes
    //  problems if we do, but that means you shouldn't use this func, or '~'
    //  in cases where the event should not be consumed.
    //
    \: (void; Event ignored) { f(); };
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

\: bind (void; string ev, VoidFunc F, string doc="") 
{ 
    bind("default", "global", ev, F, doc); 
}

\: bind (void; string ev, EventFunc F, string doc="") 
{ 
    bind("default", "global", ev, F, doc); 
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

}
