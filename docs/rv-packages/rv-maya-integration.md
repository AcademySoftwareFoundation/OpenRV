# RV/Maya Integration (v1.4)

### Requirements

The Maya integration package has only been tested with Maya 2012 and Maya 2014.

### Installation

The first step on any platform is to activate the integration package. That is, go to the *Packages* tab of the RV Preferences dialog, and click the Load button next to "Maya Tools", then restart RV.

#### Linux and Windows

On Linux and Windows, the only remaining task is to fill in the Application and Flags to run RV. In Maya open the Preferences, and go to the *Applications* section (titled "External Applications: Settings"). In the entry marked *Application Path for Viewing Sequences* enter the complete path to the *RVPUSH* executable. For example:

Linux

```
 /usr/local/bin/rv/bin/rvpush 
```

Windows

```
 C:\Program Files (x86)\RV\bin\rvpush.exe 
```

Then, in the entry marked *Optional Flags* , type this:

```
 -tag playblast merge %f 
```

#### OSX (Maya 2012 or Maya 2013)

On OSX, using Maya 2012 or Maya 2013, after turning on the Maya integration as described above and restarting RV, you need to install a tiny MEL script to handle the Maya side of the playblasting. In RV, go to the Maya menu, and select *Install Maya Support File.* (The file is installed in `Library/Preferences/Autodesk/maya/scripts` .)

Then, in Maya open the Preferences, and go to the *Applications* section (titled "External Applications: Settings"). In the entry marked *Application Path for Viewing Sequences* enter the Following:

```
 playblastWithRV 
```

Then, in the entry marked *Optional Flags* , type this:

```
 %f %r 
```

Done!

#### OSX (Maya 2014)

On OSX, using Maya 2014, after turning on the Maya integration as described above, and quitting RV, in Maya open the Preferences, go to the *Applications* section (titled "External Applications: Settings"). In the section marked *Sequence Viewing Applications* there are three applications, each with two entries, one for the application, and one for *Optional Flags* . In each application entry (assuming you’ve installed RV in the usual location), enter the following:

```
 unset QT_MAC_NO_NATIVE_MENUBAR; /Applications/RV64.app/Contents/MacOS/rvpush 
```

Then, in each entry marked *Optional Flags* , type this:

```
 -tag playblast merge [ %f -fps %r ] 
```

Done!

### Build a Session in Open RV

The main point to remember about playblasting to RV is that if you leave RV open, each successive playblast will be merged into the RV Session, so you can easily compare the current render with others in the session.

Open the Session Manager (from the *Tools* menu, or by hitting the *x* key) to see a list your playblasts and view them in different ways. Each playblast will have a name provided by Maya, plus a timestamp that RV adds to help you track them. If you want to rename a playblast to something more meaningful, just select it in the Session Manager, and select the *Edit View Info* button from the right-click menu, or hit the button with the *I* on it.

At any point, you can save your Session to a Session File, so that you can easily pick up where you left off. When you return, render one playblast to get RV going, then *File→Merge* to bring in the Session File from the previous session.

### Organizing and Comparing your Renders

By default, each new playblast that Maya adds to your session will become the current view. Note that you can easily switch back and forth between this Render and the previous one with the *shift-left/right-arrow* keys.

The "default" views (see the Session Manager) also let you easily see all your playblasts in a Sequence, in a Layout, or arranged in a Stack (note that you can use the "()" keys to easily cycle the stack, or you can use drag-and-drop in the Inputs section of the Session Manger to easily rearrange a group view.

If you’d like to "stick" to the view you’ve chosen, instead of switching to each new playblast as it arrives, toggle the *View Latest Playblast* option off on the *Maya* menu in RV.

### Creating New Views

All the usual options in the Session Manager for creating and managing new views are available for managing your playblasts, but there are also a couple of hand shortcuts on the *Maya* menu.

Wipe Selected Playblasts

Select two or more playblasts in the Session Manager and hit this item to arrange them in a Stack, with Wipes mode activated, so that you can grab the edges of the images and slide them over the lower images. Remember that the "()" keys will cycle the stack order, and you can also drag and drop in the Inputs section of the Session Manager to reorder the stack.

Tile Selected Playblasts

Select two or more playblasts in the Session Manager and hit this item to arrange them in a Tiled Layout.

### Rendering into Context

Don’t forget that you can also bring in supplementary media to compare or use as reference for your animation. So for example you might bring in takes of the shots on either side of the one you’re animating, and assemble a 3-shot sequence with one of your playblasts in the middle. But as you work, you’d like that middle shot to be replaced by newer playblasts. We call that "rendering into context".

In general, to prepare to render into context, after you setup your views, you’d

1.  Turn off the *View Latest Playblast* option in the *Maya* menu.
    
2.  Select (in the Session Manager) the previously-rendered playblast you want to swap out for new ones
    
3.  Pick the *Mark Selected as Target* item on the *Maya* menu.
    

Henceforth, future playblasts will be swapped into the slot occupied by the one you just marked. Possible "render into context" workflows include:

Your shot in the cut

Load the shots before and after yours and make a Sequence view. Note that you can trim shots by adjusting the in/out points on the timeline in the Source view.

Wipe between the latest playblast and a previous take, or reference footage

Add one or more sources (or use a previous playblast). Make a Stack view. Order the Stack to your liking by dragging and dropping in the Inputs section of the Session Manager. Turn on Wipes (Tools menu).

Tile newest playblast with one or more others, reference footage, etc

Add one or more sources (or use a previous playblast). Make a Layout view. Select Tile/Column/Row (or even arrange by hand with the Manual mode) from the *Layout* menu. Arrange the Layout to your liking by dragging and dropping in the Inputs section of the Session Manager.

* * *

Last updated 2014-04-08 12:29:03 PDT
