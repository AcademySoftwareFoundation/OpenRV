# Chapter 11 - The Custom Matte Package

Now that we've tried the simple stuff, let's do something useful. RV has a number of settings for viewing mattes. These are basically regions of the frame that are darkened or completely blackened to simulate what an audience will see when the movie is projected. The size and shape of the matte is an artistic decision and sometimes a unique matte will be required.

You can find various common mattes already built into RV under the View menu. In this example we'll create a Python package that reads a file when RV starts to get a list of matte geometry and names. We'll make a custom menu out of these which will set some state in the UI.To start with, we'll assume that the path to the file containing the mattes is located in an environment variable called RV_CUSTOM_MATTE_DEFINITIONS. We'll get the value of that variable, open and parse the file, and create a data struct holding all of the information about the mattes. If it is not defined we will provide a way for the user to locate the file through an open-file-dialog and then parse the file.

### 11.1 Creating the Package


Use the same method described in Chapter [10](rv-reference-manual-chapter-ten.md#chapter-10-a-simple-package) to begin working on the package. If you haven't read that chapter please do so first. A completed version of the package created in this chapter is included in the RV distribution. So using that as reference is a good idea.

### 11.2 The Custom Matte File


The file will be a very simple comma separated value (CSV) file. Each line starts with the name of the custom matte (shown in the menu) followed by four floating point values and then a text field description which will be displayed when that matte is activated. So each line will look something like this:

```
 matte menu name, aspect ratio, fraction of image visible, center point of matte in X, center point of matte in Y, descriptive text 
```

### 11.3 Parsing the Matte File


Before we actually parse the file, we should decide what we want when we're done. In this case we're going to make our own data structure to hold the information in each line of the file. We'll hold all of the information we collect in a Python dictionary with the following keys:

```
 "name", "ratio", "heightVisible", "centerX", "centerY", and "text" 
```
Next we'll write a method for our mode that does the parsing and updates our internal mattes dictionary.

> **Note:** If you are unfamiliar with object oriented programing you can substitute the word function for method. This manual will sometimes refer to a method as a function. It will never refer to a non-method function as a method.

```
     def updateMattesFromFile(self, filename):
        # Make sure the definition file exists
        if (not os.path.exists(filename)):
            raise KnownError("ERROR: Custom Mattes Mode: Non-existent mattes" +
                " definition file: '%s'" % filename)

        # Walk through the lines of the definition file collecting matte
        # parameters
        order = []
        mattes = {}
        for line in open(filename).readlines():
            tokens = line.strip("\n").split(",")
            if (len(tokens) == 6):
                order.append(tokens[0])
                mattes[tokens[0]] = {
                    "name" : tokens[0], "ratio" : tokens[1],
                    "heightVisible" : tokens[2], "centerX" : tokens[3],
                    "centerY" : tokens[4], "text" : tokens[5]}

        # Make sure we got some valid mattes
        if (len(order) == 0):
            self._order = []
            self._mattes = {}
            raise KnownError("ERROR: Custom Mattes Mode: Empty mattes" +
                " definition file: '%s'" % filename)

        self._order = order
        self._mattes = mattes 
```
There are a number of things to note in this function. First of all, to keep track of the order in which we read the definitions from the mattes file you will see that stored in the “_order” Python list. The “_mattes” dictionary's keys are the same as the “_order” list, but since dictionaries are not ordered we use the list to remember the order.We check to see if the file actually exists and if not simply raise a KnownError Exception. So the caller of this function will have to be ready to except a KnownError if the matte definition file cannot be found or if it is empty. The KnowError Exception is simply our own Exception class. Having our own Exception class allows us to raise and except Exceptions that we know about while letting others we don't expect to still reach the user. Here is the definition of our KnownError Exception class.

```
 class KnownError(Exception): pass 
```
We use the built-in Python readlines() method to go through the mattes file contents one line at a time. Each time through the loop, the next line is split over commas since that's how defined the fields of each line.If there are not exactly 6 tokens after splitting the line, that means the line is corrupt and we ignore it. Otherwise, we add a new dictionary to our “_mattes” dictionary of matte definition dictionaries.If we cannot find the path defined in the environment variable then we leave it blank:

```
         try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = "" 
```
At this point the custom_mattes.py file looks like this:

```
from rv import commands, rvtypes
import os

class KnownError(Exception): pass

class CustomMatteMinorMode(rvtypes.MinorMode):

    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self._order = []
        self._mattes = {}
        self._currentMatte = ""
        self.init("custom-mattes-mode", None, None, None)

        try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = ""
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst))

    def updateMattesFromFile(self, filename):

        # Make sure the definition file exists
        if (not os.path.exists(filename)):
            raise KnownError("ERROR: Custom Mattes Mode: Non-existent mattes" +
                " definition file: '%s'" % filename)

        # Walk through the lines of the definition file collecting matte
        # parameters
        order = []
        mattes = {}
        for line in open(filename).readlines():
            tokens = line.strip("\n").split(",")
            if (len(tokens) == 6):
                order.append(tokens[0])
                mattes[tokens[0]] = {
                    "name" : tokens[0], "ratio" : tokens[1],
                    "heightVisible" : tokens[2], "centerX" : tokens[3],
                    "centerY" : tokens[4], "text" : tokens[5]}

        # Make sure we got some valid mattes
        if (len(order) == 0):
            self._order = []
            self._mattes = {}
            raise KnownError("ERROR: Custom Mattes Mode: Empty mattes" +
                " definition file: '%s'" % filename)

        self._order = order
        self._mattes = mattes

def createMode():
    return CustomMatteMinorMode() 
```

### 11.4 Adding Bindings and Menus


The mode constructor needs to do three things: call the file parsing function, do something sensible if the matte file parsing fails, and build a menu with the items found in the matte file as well as add bindings to the menu items.We have already gone over the parsing. Once parsing is done we either have a good list of mattes or an empty one, but either way we move on to setting up the menus. Here is the method that will build the menus and bindings.

```
     def setMenuAndBindings(self):

        # Walk through all of the mattes adding a menu entry as well as a
        # hotkey binding for alt + index number
        # NOTE: The bindings will only matter for the first 9 mattes since you
        # can't really press "alt-10".
        matteItems = []
        bindings = []
        if (len(self._order) > 0):
            matteItems.append(("No Matte", self.selectMatte(""), "alt `",
                self.currentMatteState("")))
            bindings.append(("key-down--alt--`", ""))

            for i,m in enumerate(self._order):
                matteItems.append((m, self.selectMatte(m),
                    "alt %d" % (i+1), self.currentMatteState(m)))
                bindings.append(("key-down--alt--%d" % (i+1), m))
        else:
            def nada():
                return commands.DisabledMenuState
            matteItems = [("RV_CUSTOM_MATTE_DEFINITIONS UNDEFINED",
                None, None, nada)]

        # Always add the option to choose a new definition file
        matteItems += [("_", None)]
        matteItems += [("Choose Definition File...", self.selectMattesFile,
            None, None)]

        # Clear the menu then add the new entries
        matteMenu = [("View", [("_", None), ("Custom Mattes", None)])]
        commands.defineModeMenu("custom-mattes-mode", matteMenu)
        matteMenu = [("View", [("_", None), ("Custom Mattes", matteItems)])]
        commands.defineModeMenu("custom-mattes-mode", matteMenu)

        # Create hotkeys for each matte
        for b in bindings:
            (event, matte) = b
            commands.bind("custom-mattes-mode", "global", event,
                self.selectMatte(matte), "") 
```
You can see that creating the menus and bindings walks through the contents of our “_mattes” dictionary in the order dictated by “_order”. If there are no valid mattes found then we add the alert in the menu to the user that the environment variable was not defined. You can also see from the example above that each menu entry is set to trigger a call to selectMatte for the associated matte definition. This is a neat technique where we use a factory method to create our event handling method for each valid matte we found. Here is the content of that:

```
     def selectMatte(self, matte):

        # Create a method that is specific to each matte for setting the
        # relevant session node properties to display the matte
        def select(event):
            self._currentMatte = matte
            if (matte == ""):
                commands.setIntProperty("#Session.matte.show", [0], True)
                extra_commands.displayFeedback("Disabling mattes", 2.0)
            else:
                m = self._mattes[matte]
                commands.setFloatProperty("#Session.matte.aspect",
                    [float(m["ratio"])], True)
                commands.setFloatProperty("#Session.matte.heightVisible",
                    [float(m["heightVisible"])], True)
                commands.setFloatProperty("#Session.matte.centerPoint",
                    [float(m["centerX"]), float(m["centerY"])], True)
                commands.setIntProperty("#Session.matte.show", [1], True)
                extra_commands.displayFeedback(
                    "Using '%s' matte" % matte, 2.0)
        return select 
```
Notice that we didn't say which matte to set it to. The function just sets the value to whatever its argument is. Since this function is going to be called when the menu item is selected it needs to be an event function (a function which takes an Event as an argument and returns nothing). In the case where we want no matte drawn, we'll pass in the empty string (“”).The menu state function (which will put a check mark next to the current matte) has a similar problem. In this case we'll use a mechanism with similar results. We'll create a method which returns a function given a matte. The returned function will be our menu state function. This sounds complicated, but it's simple in use:The thing to note here is that the parameter m passed into currentMatteState() is being used inside the function that it returns. The m inside the matteState() function is known as a free variable. The value of this variable at the time that currentMatteState() is called becomes wrapped up with the returned function. One way to think about this is that each time you call currentMatteState() with a new value for m, it will return a different copy of matteState() function where the internal m is replaced the value of currentMatteState()'s m.

```
     def currentMatteState(self, m):
        def matteState():
            if (m != "" and self._currentMatte == m):
                return commands.CheckedMenuState
            return commands.UncheckedMenuState
        return matteState 
```
Selecting mattes is not the only menu option we added in setMenuAndBindings(). We also added an option to select the matte definition file (or change the selected one) if none was found before. Here is the contents of the selectMatteFile() method:

```
     def selectMattesFile(self, event):
        definition = commands.openFileDialog(True, False, False, None, None)[0]
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst))
        self.setMenuAndBindings() 
```
Notice here that we basically repeat what we did before when parsing the mattes definition file from the environment. We update our internal mattes structures and the setup the menus and bindings.It is also important to clear out any existing bindings when we load a new mattes file. Therefore we should modify our parsing function do this for us like so:

```
     def updateMattesFromFile(self, filename):

        # Make sure the definition file exists
        if (not os.path.exists(filename)):
            raise KnownError("ERROR: Custom Mattes Mode: Non-existent mattes" +
                " definition file: '%s'" % filename)

        # Clear existing key bindings
        for i in range(len(self._order)):
            commands.unbind(
                "custom-mattes-mode", "global", "key-down--alt--%d" % (i+1))

          ... THE REST IS AS BEFORE ... 
```
So the full mode constructor function now looks like this:

```
 class CustomMatteMinorMode(rvtypes.MinorMode):

    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self._order = []
        self._mattes = {}
        self._currentMatte = ""
        self.init("custom-mattes-mode", None, None, None)

        try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = ""
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst)) 
```

### 11.5 Handling Settings


Wouldn't it be nice to have our package remember what our last matte setting was and where the last definition file was? Lets see how to add settings. First thing is first. We need to write our settings in order to read them back later. Lets start by writing out the location of our mattes definition file when we parse a new one. Here is an updated version of updateMattesFromFile():

```
     def updateMattesFromFile(self, filename):

        # Make sure the definition file exists
        if (not os.path.exists(filename)):
            raise KnownError("ERROR: Custom Mattes Mode: Non-existent mattes" +
                " definition file: '%s'" % filename)

        # Clear existing key bindings
        for i in range(len(self._order)):
            commands.unbind(
                "custom-mattes-mode", "global", "key-down--alt--%d" % (i+1))

        # Walk through the lines of the definition file collecting matte
        # parameters
        order = []
        mattes = {}
        for line in open(filename).readlines():
            tokens = line.strip("\n").split(",")
            if (len(tokens) == 6):
                order.append(tokens[0])
                mattes[tokens[0]] = {
                    "name" : tokens[0], "ratio" : tokens[1],
                    "heightVisible" : tokens[2], "centerX" : tokens[3],
                    "centerY" : tokens[4], "text" : tokens[5]}

        # Make sure we got some valid mattes
        if (len(order) == 0):
            self._order = []
            self._mattes = {}
            raise KnownError("ERROR: Custom Mattes Mode: Empty mattes" +
                " definition file: '%s'" % filename)

        # Save the definition path and assign the mattes
        commands.writeSettings(
            "CUSTOM_MATTES", "customMattesDefinition", filename)
        self._order = order
        self._mattes = mattes 
```
See how at the bottom of the function we are now writting the definition file to the CUSTOM_MATTES settings. Now lets also update the selectMatte() method to remember what matte we selected.

```
     def selectMatte(self, matte):

        # Create a method that is specific to each matte for setting the
        # relevant session node properties to display the matte
        def select(event):
            self._currentMatte = matte
            if (matte == ""):
                commands.setIntProperty("#Session.matte.show", [0], True)
                extra_commands.displayFeedback("Disabling mattes", 2.0)
            else:
                m = self._mattes[matte]
                commands.setFloatProperty("#Session.matte.aspect",
                    [float(m["ratio"])], True)
                commands.setFloatProperty("#Session.matte.heightVisible",
                    [float(m["heightVisible"])], True)
                commands.setFloatProperty("#Session.matte.centerPoint",
                    [float(m["centerX"]), float(m["centerY"])], True)
                commands.setIntProperty("#Session.matte.show", [1], True)
                extra_commands.displayFeedback(
                    "Using '%s' matte" % matte, 2.0)
            commands.writeSettings("CUSTOM_MATTES", "customMatteName", matte)
        return select 
```
Here notice the second to last line. We save the matte that was just selected. Lastly lets see what we have to do to make use of these when we initialize our mode. Here is the final version of the constructor:

```
 class CustomMatteMinorMode(rvtypes.MinorMode):

    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self._order = []
        self._mattes = {}
        self._currentMatte = ""
        self.init("custom-mattes-mode", None, None, None)

        try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = str(commands.readSettings(
                "CUSTOM_MATTES", "customMattesDefinition", ""))
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst))
        self.setMenuAndBindings()

        lastMatte = str(commands.readSettings(
            "CUSTOM_MATTES", "customMatteName", ""))
        for matte in self._order:
            if matte == lastMatte:
                self.selectMatte(matte)(None) 
```
Here we grab the last known location of the mattes definition file if we did not find one in the environment. We also attempt to look up the last matte that was used and if we can find it among the mattes we parsed then we enable that selection.

### 11.6 The Finished custom_mattes.py File


```
from rv import commands, rvtypes, extra_commands
import os

class KnownError(Exception): pass

class CustomMatteMinorMode(rvtypes.MinorMode):

    def __init__(self):
        rvtypes.MinorMode.__init__(self)
        self._order = []
        self._mattes = {}
        self._currentMatte = ""
        self.init("custom-mattes-mode", None, None, None)

        try:
            definition = os.environ["RV_CUSTOM_MATTE_DEFINITIONS"]
        except KeyError:
            definition = str(commands.readSettings(
                "CUSTOM_MATTES", "customMattesDefinition", ""))
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst))
        self.setMenuAndBindings()

        lastMatte = str(commands.readSettings(
            "CUSTOM_MATTES", "customMatteName", ""))
        for matte in self._order:
            if matte == lastMatte:
                self.selectMatte(matte)(None)

    def currentMatteState(self, m):
        def matteState():
            if (m != "" and self._currentMatte == m):
                return commands.CheckedMenuState
            return commands.UncheckedMenuState
        return matteState

    def selectMatte(self, matte):

        # Create a method that is specific to each matte for setting the
        # relevant session node properties to display the matte
        def select(event):
            self._currentMatte = matte
            if (matte == ""):
                commands.setIntProperty("#Session.matte.show", [0], True)
                extra_commands.displayFeedback("Disabling mattes", 2.0)
            else:
                m = self._mattes[matte]
                commands.setFloatProperty("#Session.matte.aspect",
                    [float(m["ratio"])], True)
                commands.setFloatProperty("#Session.matte.heightVisible",
                    [float(m["heightVisible"])], True)
                commands.setFloatProperty("#Session.matte.centerPoint",
                    [float(m["centerX"]), float(m["centerY"])], True)
                commands.setIntProperty("#Session.matte.show", [1], True)
                extra_commands.displayFeedback(
                    "Using '%s' matte" % matte, 2.0)
            commands.writeSettings("CUSTOM_MATTES", "customMatteName", matte)
        return select

    def selectMattesFile(self, event):
        definition = commands.openFileDialog(True, False, False, None, None)[0]
        try:
            self.updateMattesFromFile(definition)
        except KnownError,inst:
            print(str(inst))
        self.setMenuAndBindings()

    def setMenuAndBindings(self):

        # Walk through all of the mattes adding a menu entry as well as a
        # hotkey binding for alt + index number
        # NOTE: The bindings will only matter for the first 9 mattes since you
        # can't really press "alt-10".
        matteItems = []
        bindings = []
        if (len(self._order) > 0):
            matteItems.append(("No Matte", self.selectMatte(""), "alt `",
                self.currentMatteState("")))
            bindings.append(("key-down--alt--`", ""))

            for i,m in enumerate(self._order):
                matteItems.append((m, self.selectMatte(m),
                    "alt %d" % (i+1), self.currentMatteState(m)))
                bindings.append(("key-down--alt--%d" % (i+1), m))
        else:
            def nada():
                return commands.DisabledMenuState
            matteItems = [("RV_CUSTOM_MATTE_DEFINITIONS UNDEFINED",
                None, None, nada)]

        # Always add the option to choose a new definition file
        matteItems += [("_", None)]
        matteItems += [("Choose Definition File...", self.selectMattesFile,
            None, None)]

        # Clear the menu then add the new entries
        matteMenu = [("View", [("_", None), ("Custom Mattes", None)])]
        commands.defineModeMenu("custom-mattes-mode", matteMenu)
        matteMenu = [("View", [("_", None), ("Custom Mattes", matteItems)])]
        commands.defineModeMenu("custom-mattes-mode", matteMenu)

        # Create hotkeys for each matte
        for b in bindings:
            (event, matte) = b
            commands.bind("custom-mattes-mode", "global", event,
                self.selectMatte(matte), "")

    def updateMattesFromFile(self, filename):

        # Make sure the definition file exists
        if (not os.path.exists(filename)):
            raise KnownError("ERROR: Custom Mattes Mode: Non-existent mattes" +
                " definition file: '%s'" % filename)

        # Clear existing key bindings
        for i in range(len(self._order)):
            commands.unbind(
                "custom-mattes-mode", "global", "key-down--alt--%d" % (i+1))

        # Walk through the lines of the definition file collecting matte
        # parameters
        order = []
        mattes = {}
        for line in open(filename).readlines():
            tokens = line.strip("\n").split(",")
            if (len(tokens) == 6):
                order.append(tokens[0])
                mattes[tokens[0]] = {
                    "name" : tokens[0], "ratio" : tokens[1],
                    "heightVisible" : tokens[2], "centerX" : tokens[3],
                    "centerY" : tokens[4], "text" : tokens[5]}

        # Make sure we got some valid mattes
        if (len(order) == 0):
            self._order = []
            self._mattes = {}
            raise KnownError("ERROR: Custom Mattes Mode: Empty mattes" +
                " definition file: '%s'" % filename)

        # Save the definition path and assign the mattes
        commands.writeSettings(
            "CUSTOM_MATTES", "customMattesDefinition", filename)
        self._order = order
        self._mattes = mattes

def createMode():
    return CustomMatteMinorMode() 
```