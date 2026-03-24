# Chapter 4 - Python

Which language should you use to customize RV? In short, we recommend using Python. You can use Python3 in RV in conjunction with Mu, or in place of it. It's even possible to call Python commands from Mu and vice versa. Python is a full peer to Mu as far as RV is concerned.

_Note: there are some slight differences that need to be noted when translating code between the two languages: In Python the modules names required by RV are the same as in Mu. As of this writing, these are commands, extra_commands, rvtypes, and rvui. However, the Python modules all live in the rv package. This package is in-memory and only available at RV's runtime. You can access these commands via writing your own custom MinorMode package._ So while in Mu, you can:

```
 use commands 
```

```
 require commands 
```

to make the commands visible in the current namespace. In Python you need to include the package name:

```python
 from rv.commands import * 
```

or

```python
 import rv.commands 
```

### Open RV Python quickstart

In order to extend RV using Python you will be making a "mode" as part of an rvpkg package—this is identical to the way it’s done in Mu and this is the method that we use internally to add new functions to RV's interface. Creation of a modes and packages is documented later in this chapter. Here is a very simple mode written in Python which is part of the RV packages as `pyhello-1.1.rvpkg`.

```python
import rv.rvtypes  
import rv.commands

class PyHello(rv.rvtypes.MinorMode):  
    "A simple example that shows how to make shift-Z start/stop playback"

    def togglePlayback(self, event):  
        if rv.commands.isPlaying():  
            rv.commands.stop()  
        else:  
            rv.commands.play()

    def __init__(self):  
        rv.rvtypes.MinorMode.__init__(self)  
        self.init("pyhello",  [("key-down--Z", self.togglePlayback, "Z key")],  None)

    def createMode():  
        "Required to initialize the module. RV will call this function to create your mode."  
        return PyHello()
```

#### Documentation

The command API is nearly identical to Mu. There are a few modules which are important to know about: `rv.rvtypes`, `rv.commands`, `rv.extra_commands`, `and rv.rvui`. These implement the base Python interface to RV.

There is no separate documentation for RV's command API in Python (e.g., via Pydoc), but you can use the existing Mu Command API Browser available under RV's Help menu. The commands and extra_commands modules are basically identical between the two languages.

### 4.1 Calling Mu From Python

It's possible to call Mu code from Python, but in practice you will probably not need to do this unless you need to interface with existing packages written in Mu. To call a Mu function from Python, you need to import the MuSymbol type from the pymu module. In this example, the play function is imported and called F on the Python side. F is then executed:

```python
from pymu import MuSymbol
F = MuSymbol("commands.play")
F() 
```

If the Mu function has arguments you supply them when calling. Return values are automatically converted between languages. The conversions are indicated in Figure [4.3](#43-python-mu-type-conversions).

```python
from pymu import MuSymbol
F = MuSymbol("commands.isPlaying")
G = MuSymbol("commands.setWindowTitle")
if F() == True:
    G("PLAYING") 
```

Once a MuSymbol object has been created, the overhead to call it is minimal. All of the Mu commands module is imported on start up or reimplemented as native CPython in the Python rv.commands module so you will not need to create MuSymbol objects yourself; just import rv.commands and use the pre-existing ones.

When a Mu function parameter takes a class instance, a Python dictionary can be passed in. When a Mu function returns a class, a dictionary will be returned. Python dictionaries should have string keys which have the same names as the Mu class fields and corresponding values of the correct types. For example, the Mu class Foo { int a; float b; } as instantiated as Foo(1, 2.0) will be converted to the Python dictionary {'a' : 1, 'b' : 2.0} and vice versa. Existing Mu code can be leveraged with the rv.runtime.eval call to evaluate arbitrary Mu from Python. The second argument to the eval function is a list of Mu modules required for the code to execute and the result of the evaluation will be returned as a string. For example, here's a function that could be a render method on a mode; it uses the Mu gltext module to draw the name of each visible source on the image:

```python
 def myRender (event) :
    event.reject()

    for s in rv.commands.renderedImages() :
        if (rv.commands.nodeType(rv.commands.nodeGroup(s["node"])) != "RVSourceGroup") :
            continue
        geom    = rv.commands.imageGeometry(s["name"])

        if (len(geom) == 0) :
            continue

        x       = geom[0][0]
        y       = (geom[0][1] + geom[2][1]) / 2.0
        domain  = event.domain()
        w       = domain[0]
        h       = domain[1]

        drawCode = """
       {
           rvui.setupProjection (%d, %d);
           gltext.color (rvtypes.Color(1.0,1.0,1.0,1));
           gltext.size(14);
           gltext.writeAt(%f, %f, extra_commands.uiName("%s"));
       }
       """
       rv.runtime.eval(drawCode % (w, h, float(x), float(y), s["node"]), ["rvui", "rvtypes", "extra_commands"]) 
```

> **Note:** Python code in RV can assume that default parameters in Mu functions will be supplied if needed.

### 4.2 Calling Python From Mu

There are two ways to call Python from Mu code: a Python function being used as a call back function from Mu or via the "python" Mu module. In order to use a Python callable object as a call back from Mu code simply pass the callable object to the Mu function. The call back function's arguments will be converted according to the Mu to Python value conversion rules show in Figure [4.3](#43-python-mu-type-conversions) . There are restrictions on which callable objects can be used; only callable objects which return values of None, Float, Int, String, Unicode, Bool, or have no return value are currently allowed. Callable objects which return unsupported values will cause a Mu exception to be thrown after the callable returns. The Mu "python" module implements a small subset of the CPython API. You can see documentation for this module in the Mu Command API Browser under the Help menu. Here is an example of how you would call os.path.join from Python in Mu.

```
require python;

let pyModule = python.PyImport_Import ("os");

python.PyObject pyMethod = python.PyObject_GetAttr (pyModule, "path");
python.PyObject pyMethod2 = python.PyObject_GetAttr (pyMethod, "join");

string result = to_string(python.PyObject_CallObject (pyMethod2, ("root","directory","subdirectory","file")));

print("result: %s\n" % result); // Prints "result: root/directory/subdirectory/file" 
```

If the method you want to call takes no arguments like os.getcwd, then you will want to call it in the following manner.

```
require python;

let pyModule = python.PyImport_Import ("os");

python.PyObject pyMethod = python.PyObject_GetAttr (pyModule, "getcwd");

string result = to_string(python.PyObject_CallObject (pyMethod, python.PyTuple_New(0)));

print("result: %s\n" % result); // Prints "result: /var/tmp" 
```

If the method you want to call require the python class instance "self" as an argument, you can get it by using the ModeManager as in the following exemple

```
let pyModule = python.PyImport_Import ("sgtk_bootstrap");  
python.PyObject pyMethod = python.PyObject_GetAttr (pyModule, "ToolkitBootstrap");  
python.PyObject pyMethod2 = python.PyObject_GetAttr (pyMethod, "queue_launch_import_cut_app");  
  
State state = data();  
ModeManagerMode manager = state.modeManager;  
ModeManagerMode.ModeEntry entry = manager.findModeEntry ("sgtk_bootstrap");  
  
if (entry neq nil)  
{  
   PyMinorMode sgtkMode = entry.mode;  
   python.PyObject_CallObject (pyMethod2, (sgtkMode._pymode, "no event"));  
}
```

If you are interested in retrieving an attribute alone then here is an example of how you would call sys.platform from Python in Mu.

```
require python;

let pyModule = python.PyImport_Import ("sys");

python.PyObject pyAttr = python.PyObject_GetAttr (pyModule, "platform");

string result = to_string(pyAttr);

print("result: %s\n" % result); // Prints "result: darwin" 
```

### 4.3 Python Mu Type Conversions

| Python Type | Converts to Mu Type | Converts To Python Type | |
| --- | --- | --- | --- |
| Str or Unicode | string | Unicode string | Normal byte strings and unicode strings are both converted to Mu's unicode string. Mu strings always convert to unicode Python strings. |
| Int | int, short, or byte | Int | |
| Long | int64 | Long | |
| Float | float or half or double | Float | Mu double values may lose precision. Python float values may lose precision if passed to a Mu function that takes a half. |
| Bool | bool | Bool | |
| (Float, Float) | vector float[2] | (Float, Float) | Vectors are represented as tuples in Python |
| (Float, Float, Float) | vector float[3] | (Float, Float, Float) | |
| (Float, Float, Float, Float) | vector float[4] | (Float, Float, Float, Float) | |
| Event | Event | Event | |
| MuSymbol | runtime.symbol | MuSymbol | |
| Tuple | tuple | Tuple | Tuple elements each convert independently. NOTE: two to four element Float tuples will convert to vector float[N] in Mu. Currently there is no way to force conversion of these Float-only tuples to Mu float tuples. |
| List | type[] or type[N] | List | Arrays (Lists) convert back and forth |
| Dictionary | Class | Dictionary | Class labels become dictionary keys |
| Callable Object | Function Object | Not Applicable | Callable objects may be passed to Mu functions where a Mu function type is expected. This allows Python functions to be used as Mu call back functions. |

Table 4.1:Mu-Python Value Conversion

### 4.4 PySide Example

You can use PySide2 to make Qt interface components (RV is a Qt Application). Below is a simple pyside example using RV's py-interp.

```python
#!/Applications/RV.app/Contents/MacOS/py-interp

# Import PySide2 classes
import sys
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

# Create a Qt application.
# IMPORTANT: RV's py-interp contains an instance of QApplication;
# so always check if an instance already exists.
app = QApplication.instance()
if app == None:
    app = QApplication(sys.argv)

# Display the file path of the app.
print(f"{app.applicationFilePath()}")

# Create a Label and show it.
label = QLabel("Using RV's PySide2")
label.show()

# Enter Qt application main loop.
app.exec_()

sys.exit() 
```

To access RV's essential session window Qt QWidgets, i.e. the main window, the GL view, top tool bar and bottom tool bar, import the Python module 'rv.qtutils'.

```python
import rv.qtutils

# Gets the current RV session windows as a PySide QMainWindow.
rvSessionWindow = rv.qtutils.sessionWindow()

# Gets the current RV session GL view as a PySide QGLWidget.
rvSessionGLView = rv.qtutils.sessionGLView()

# Gets the current RV session top tool bar as a PySide QToolBar.
rvSessionTopToolBar = rv.qtutils.sessionTopToolBar()

# Gets the current RV session bottom tool bar as a PySide QToolBar.
rvSessionBottomToolBar = rv.qtutils.sessionBottomToolBar() 
```

### 4.5 Open RV Python Implementation FAQ

#### Can I draw on the view the way Mu does using OpenGL?

Yes. If you bind to a render event you can draw using PyOpenGL.

#### Module XXX is missing. Where can I get it?  

Use `py-interp -m pip` to get the missing package, like you would any other python package.

If the module is not included and it’s a CPython module (written in C) you will need to compile it yourself. The compiled module must be added to the `Python` plug-ins folder.

#### The commands.bind() function in Python doesn’t work the same way as in Mu? How do I use it?  

Python currently requires all arguments to bind(). So to make it the "short form" do:  

```python
bind("default", "global", event, func, event_doc_string).  
```

#### Does the Python <-> Mu bridge slow things down?

The Python <-> Mu bridge does not slow things down. The MuSymbol type used to interface between them completely skips interpreted Mu code if it’s calling a "native" Mu function from Python. All of the RV commands are native Mu functions. So there’s a thin layer between the Python call and the actual underlying RV command (which is largely language agnostic).

The Mu calling into Python bridge is roughly the cost of calling a Python function from C.  

## Why does my external Python process (which I call from Open RV) now behave differently?  

This is probably caused by the fact that RV modifies the PYTHONPATH to incorporate the Python plug-in folder and RV's python standard libraries to run. Forked processes will inherit the PYTHONPATH. If you are using QProcess to launch the external process you can call `QProcess.setEnvironment()` to set the PYTHONPATH before calling `QProcess.start()`.
