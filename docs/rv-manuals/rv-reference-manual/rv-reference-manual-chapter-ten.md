# Chapter 10 - A Simple Package

This first example will show how to create a package that defines some key bindings and creates a custom personal menu. You will not need to edit a .rvrc.mu file to do this as in previous versions.We'll be creating a package intended to keep all our personal customizations. To start with we'll need to make a Mu module that implements a new mode. At first won't do anything at all: just load at start up. Put the following in to a file called mystuff.mu.

```
use rvtypes;
use extra_commands;
use commands;

module: mystuff {

class: MyStuffMode : MinorMode
{
    method: MyStuffMode (MyStuffMode;)
    {
        init("mystuff-mode",
             nil,
             nil,
             nil);
    }
}

\: createMode (Mode;)
{
    return MyStuffMode();
}

} // end module
 
```

Now we need to create a PACKAGE file in the same directory before we can create the package zip file. It should look like this:

```
package: My Stuff
author: M. VFX Artiste
version: 1.0
rv: 3.6
requires: ''

modes:
  - file: mystuff
    load: immediate

description: |
  <p>M. VFX Artiste's Personal RV Customizations</p> 
```

Assuming both files are in the same directory, we create the zip file using this command from the shell:

```
 shell> zip mystuff-1.0.rvpkg PACKAGE mystuff.mu 
```

The file mystuff-1.0.rvpkg should have been created. Now start RV, open the preferences package pane and add the mystuff-1.0.rvpkg package. You should now be able to install it. Make sure the package is both installed and loaded in your home directory's RV support directory so it's private to you.At this point, we'll edit the installed Mu file directly so we can see results faster. When we have something we like, we'll copy it back to the original mystuff.mu and make the rvpkg file again with the new code. Be careful not to uninstall the mystuff package while we're working on it or our changes will be lost. Alternately, for the more paranoid (and wiser), we could edit the file elsewhere and simply copy it onto the installed file.To start with let's add two functions on the \`\`<'' and \`\`>'' keys to speed up and slow down the playback by increasing and decreasing the FPS. There are two main this we need to do: add two method to the class which implement speeding up and slowing down, and bind those functions to the keys.First let's add the new methods after the class constructor MyStuffMode() along with two global bindings to the \`\`<'' and \`\`>'' keys. The class definition should now look like this:

```
 ...

class: MyStuffMode : MinorMode
{
    method: MyStuffMode (MyStuffMode;)
    {
        init("mystuff-mode",
             [("key-down-->", faster, "speed up fps"),
               ("key-down--<", slower, "slow down fps")],
             nil,
             nil);
    }

method: faster (void; Event event)
    {
        setFPS(fps() \* 1.5);
        displayFeedback("%g fps" % fps());
    }

method: slower (void; Event event)
    {
        setFPS(fps() \* 1.0/1.5);
        displayFeedback("%g fps" % fps());
    }
} 
```

The bindings are created by passing a list of tuples to the init function. Each tuple contains three elements: the event name to bind to, the function to call when it is activated, and a single line description of what it does. In Mu a tuple is formed by putting parenthesis around comma separated elements. A list is formed by enclosing its elements in square brackets. So a list of tuples will have the form:

```
 [ (...), (...), ... ] 
```

Where the \`\`...'' means \`\`and so on''. The first tuple in our list of bindings is:

```
 (key-down-->, faster, speed up fps) 
```

So the event in this case is key-down–> which means the point at which the > key is pressed. The symbol faster is referring to the method we declared above. So faster will be called whenever the key is pressed. Similarily we bind slower (from above as well) to key-down–<.

```
 ("key-down--<", slower, "slow down fps") 
```

And to put them in a list requires enclose the two of them in square brackets:

```
 [("key-down-->", faster, "speed up fps"),
 ("key-down--<", slower, "slow down fps")] 
```

To add more bindings you create more methods to bind and add additional tuples to the list.The python version of above looks like this:

```
from rv.rvtypes import *
from rv.commands import *
from rv.extra_commands import *

class PyMyStuffMode(MinorMode):

   def __init__(self):
        MinorMode.__init__(self)
        self.init("py-mystuff-mode",
                  [ ("key-down-->", self.faster, "speed up fps"),
                    ("key-down--<", self.slower, "slow down fps") ],
                  None,
                  None)

    def faster(self, event):
        setFPS(fps() * 1.5)
        displayFeedback("%g fps" % fps(), 2.0);

    def slower(self, event):
        setFPS(fps() * 1.0/1.5)
        displayFeedback("%g fps" % fps(), 2.0);


def createMode():
    return PyMyStuffMode() 
```

### 10.1 How Menus Work

Adding a menu is fairly straightforward if you understand how to create a MenuItem. There are different types of MenuItems: items that you can select in the menu and cause something to happen, or items that are themselves menus (sub-menu). The first type is constructed using this constructor (shown here in prototype form) for Mu:

```
 MenuItem(string       label,
         (void;Event) actionHook,
         string       key,
         (int;)       stateHook); 
```

or in Python this is specified as a tuple:

```
 ("label", actionHook, "key", stateHook) 
```

The actionHook and stateHook arguments need some explanation. The other two (the label and key) are easier: the label is the text that appears in the menu item and the key is a hot key for the menu item.The actionHook is the purpose of the menu item–it is a function or method which will be called when the menu item is activated. This is just like the method we used with bind() — it takes an Event object. If actionHook is nil, than the menu item won't do anything when the user selects it.The stateHook provides a way to check whether the menu item should be enabled (or greyed out)–it is a function or method that returns an int. In fact, it is really returning one of the following symbolic constants: NeutralMenuState, UncheckMenuState, CheckedMenuState, MixedStateMenuState, or DisabledMenuState. If the value of stateHook is nil, the menu item is assumed to always be enabled, but not checked or in any other state.A sub-menu MenuItem can be create using this constructor in Mu:

```
 MenuItem(string     label,
         MenuItem[] subMenu); 
```

or a tuple of two elements in Python:

```
 ("label", subMenu) 
```

The subMenu is an array of MenuItems in Mu or a list of menu item tuples in Python. Usually we'll be defining a whole menu — which is an array of MenuItems. So we can use the array initialization syntax to do something like this:

```
 let myMenu = MenuItem {"My Menu", Menu {
    {"Menu Item", menuItemFunc, nil, menuItemState},
    {"Other Menu Item", menuItemFunc2, nil, menuItemState2}
}} 
```

Finally you can create a sub-menu by nesting more MenuItem constructors in the subMenu.

```
 MenuItem myMenu = {"My Menu", Menu {
        {"Menu Item", menuItemFunc, nil, menuItemState},
        {"Other Menu Item", menuItemFunc2, nil, menuItemState2},
        {"Sub-Menu", Menu {
             {"First Sub-Menu Item", submenuItemFunc1, nil, submenu1State}
        }}
    }}; 
```

in Python this looks like:

```
 ("My Menu", [
  ("Menu Item", menuItemFunc, None, menuItemState),
  ("Other Menu Item", menuItemFunc2, None, menuItemState2)]) 
```

You'll see this on a bigger scale in the rvui module where most the menu bar is declared in one large constructor call.

### 10.2 A Menu in MyStuffMode

Now back to our mode. Let's say we want to put our faster and slower functions on menu items in the menu bar. The fourth argument to the init() function in our constructor takes a menu representing the menu bar. You only define menus which you want to either modify or create. The contents of our main menu will be merged into the menu bar.By merge into we mean that the menus with the same name will share their contents. So for example if we add the File menu in our mode, RV will not create a second File menu on the menu bar; it will add the contents of our File menu to the existing one. On the other hand if we call our menu MyStuff RV will create a brand new menu for us (since presumably MyStuff doesn't already exist). This algorithm is applied recursively so sub-menus with the same name will also be merged, and so on.So let's add a new menu called MyStuff with two items in it to control the FPS. In this example, we're only showing the actual init() call from mystuff.mu:

```
 init("mystuff-mode",
     [ ("key-down-->", faster, "speed up fps"),
       ("key-down--<", slower, "slow down fps") ],
     nil,
     Menu {
         {"MyStuff", Menu {
                 {"Increase FPS", faster, nil},
                 {"Decrease FPS", slower, nil}
             }
         }
     }); 
```

Normally RV will place the new menu (called \`\`MyStuff'') just before the Windows menu.If we wanted to use menu accelerators instead of (or in addition to) the regular event bindings we add those in the menu item constructor. For example, if we wanted to also use the keys - and = for slower and faster we could do this:

```
 init("mystuff-mode",
     [ ("key-down-->", faster, "speed up fps"),
       ("key-down--<", slower, "slow down fps") ],
     nil,
     Menu {
         {"MyStuff", Menu {
                 {"Increase FPS", faster, "="},
                 {"Decrease FPS", slower, "-"}
             }
         }
     }); 
```

The advantage of using the event bindings instead of the accelerator keys is that they can be overridden and mapped and unmapped by other modes and \`\`chained'' together. Of course we could also use > and < for the menu accelerator keys as well (or instead of using the event bindings).The Python version of the script might look like this:

```
from rv.rvtypes import *
from rv.commands import *
from rv.extra_commands import *

class PyMyStuffMode(MinorMode):

   def __init__(self):
        MinorMode.__init__(self)
        self.init("py-mystuff-mode",
                  [ ("key-down-->", self.faster, "speed up fps"),
                    ("key-down--<", self.slower, "slow down fps") ],
                  None,
                  [ ("MyStuff",
                     [ ("Increase FPS", self.faster, "=", None),
                       ("Decrease FPS", self.slower, "-", None)] )] )

    def faster(self, event):
        setFPS(fps() * 1.5)
        displayFeedback("%g fps" % fps(), 2.0);

    def slower(self, event):
        setFPS(fps() * 1.0/1.5)
        displayFeedback("%g fps" % fps(), 2.0);


def createMode():
    return PyMyStuffMode() 
```

### 10.3 Finishing up

Finally, we'll create the final rvpkg package by copying mystuff.mu back to our temporary directory with the PACKAGES file where we originally made the rvpkg file.Next start RV and uninstall and remove the mystuff package so it no longer appears in the package manager UI. Once you've done this recreate the rvpkg file from scratch with the new mystuff.mu file and the PACKAGES file:

```bash
 shell> zip mystuff-1.0.rvpkg PACKAGES mystuff.mu 
```

or if you're using python:

```bash
 shell> zip mystuff-1.0.rvpkg PACKAGES mystuff.py 
```

You can now add the latest mysuff-1.0.rvpkg file back to RV and use it. In the future add personal customizations directly to this package and you'll always have a single file you can install to customize RV.
