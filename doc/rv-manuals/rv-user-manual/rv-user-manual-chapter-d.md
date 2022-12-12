# D - Using Open RV as Nuke's Flip Book Player

Nuke can easily be configured to use RV as a flipbook. Nuke can also be set up to render out OpenEXR temp files instead of the default rgb files.

### D.1 Setting up a custom plugins area for Nuke


In order to configure Nuke to work with RV, you should set up a custom scripts/plugins directory where you can add new custom Nuke functionality without disturbing the default Nuke install. This directory can be anywhere on the NUKE_PATH environment variable, but note that the \`\`$HOME/.nuke'' directory is always on that NUKE_PATH, so we'll assume we're working there for now.

1. In your $HOME/.nuke directory, create a file called \`\`init.py'', if it doesn't already exist, and also one called \`\`menu.py''.

2. Add this line to the init.py file

```
nuke.pluginAddPath('./python') 
```

3. Create the directory referenced by the line we just added

```
mkdir $HOME/.nuke/python
```

Done! Now Nuke well pick up new stuff from this area on start-up

### D.2 Adding Open RV support in the custom plugins area


The following assumes that you've setup a custom plugin area as described in the previous section.

1. Download the script \`\`rv_this.py'' from [this forum post](../../rv-packages/rv-nuke-integration.md).

2. Move rv_this.py into $HOME/.nuke/python.

3. Add the following line to the $HOME/.nuke/init.py file, **after** the pluginAddPath lines mentioned above:

```
import rv_this
```

4. Add the following to the $HOME/.nuke/menu.py file:

```
menubar = nuke.menu("Nuke");
menubar.addCommand("File/Flipbook Selected in &RV",
        "nukescripts.flipbook(rv_this.rv_this, nuke.selectedNode())", "#R") 
```

RV will now appear in the render menu and will be available with the hot key Alt+r.

To add an option to render your flipbook images in Open EXR,

1. Find the file \`\`nukescripts/flip.py'' in your Nuke install and copy it to $HOME/.nuke/python/flipEXR.py.

2. Edit $HOME/.nuke/python/flipEXR.py and find the two lines that specify the output file name (search for \`\`nuke_tmp_flip'') and change the file extension from \`\`rgb'' to \`\`exr''.

3. Add the following line to the $HOME/.nuke/menu.py file:

```
menubar.addCommand("File/Flipbook Selected in &RV (EXR)",
        "flipExr.flipbook(rv_this.rv_this, nuke.selectedNode())", "#E") 
```

Now you'll have the option in the File menu to flipbook to EXR, with the hotkey Alt+e.

You can edit the rv_this.py script to specify any rv options you wish to set as your viewing defaults. For example you could un-comment the script lines that will apply -gamma 2.2 or enable -sRGB, or you could specify a file or display LUT for RV to use.