# Nuke/Open RV integration

## Introduction

Rather than just attach a “flipbook” to Nuke, the goal of this integration effort is to provide compositors with a unified framework in which RV’s core media functionality (playback, browsing, arranging, editing, etc) is always instantly available to augment and enhance Nuke’s own capabilities.

Key features include:

*   Checkpointing: Save a rendered frame with a copy of the current nuke script
    
*   Rendering: Save a rendered sequence with a copy of the current nuke script
    
*   Background rendering in Nuke 6.2 **and** 6.1
    
*   Live update of RV during renders, showing the latest frame rendered
    
*   Rendered frames visible in RV as soon as they are written
    
*   Rendered frames from canceled renders are visible
    
*   Render directly into a slap comp or sequence in RV
    
*   Full checkpoints: copies of entire ranges of frames, for comparison
    
*   Visual browsing of checkpoints and renders
    
*   Visual comparison (wipes, tiled) of checkpoints and renders
    
*   Restoring the script to the state of any checkpoint or render
    
*   Read and Write nodes in the script dynamically mirrored as sources in RV
    
*   Read/Write node path, frame range, color space dynamically synced to RV
    
*   Node selection dynamically synced to the View in RV
    
*   Frame changes in Nuke dynamically synced to RV frame
    
*   RV Sources can be used to create the corresponding Nuke Read node
    
*   All Render/Checkpoint context retained in session file on disk
    
*   Support for `%V` -style stereoscopic Reads, Writes, renders, and checkpoints.
    

## Note to Users


Thanks for trying out the software; the integration toolset is in active development, and we’d very much appreciate any bug reports, feature requests, or other comments.

Before you send us bug reports and feature requests, however, you might want to check over the list of known issues and planned work in [this appendix](#appendix-known-issues-and-planned-work) .


## Updating an Existing Installation

If this Package is updated within a new Open RV distribution, you might need to update the Python code on the “Nuke side”. To do so, just follow the Installation instructions below.

If the RV and Nuke components of the integration package are mis-matched, you’ll get an error dialog when you start RV from Nuke.

## Installation


### Personal Installation

1.  Start RV and go to the *Packages* tab of the *Preferences* dialog
    
2.  Find *Nuke Integration* in the Package list and click the *Load* toggle next to it.
    
3.  Restart RV
    
4.  Click the *Nuke* item on the *Tools* menu
    
5.  From the *Nuke* menu, select the *Install Nuke Support Files* item and follow the directions.
    

To confirm that the Nuke support files are properly installed, start Nuke. You should see an *RV* menu on the main menubar, and if you select *RV/Preferences…* , you should get the appropriate dialog.

That’s it for installation!

### Site-wide Installation

In what follows we suppose that you’ve installed RV in `/usr/local/RV` , and that you keep your Nuke scripts in subdirectories of `/usr/local/nuke/scripts` . If you do otherwise please adjust the paths below appropriately.

1.  Make a subdir in your Nuke scripts area for the rvnuke support files:
    
    ```
     % mkdir /usr/local/nuke/scripts/rvnuke 
    ```
    
2.  Copy the Nuke support files into place
    
    ```
     % cp /usr/local/rv/plugins/SupportFiles/rvnuke/* /usr/local/nuke/scripts/rvnuke 
    ```
    
3.  Edit the `init.py` file in `/usr/local/nuke/scripts` to include this line:
    
    ```
     nuke.pluginAddPath('./rvnuke') 
    ```
    

Done!

## Getting Started


### Rv Preferences

In order to launch RV from Nuke, Nuke needs to know where the RV executable is. To set this, start Nuke and select the *RV/Preferences…* menu item. Navigate to the RV executable you want to use with Nuke and hit *OK* .

This setting is stored and used across all future Nuke sessions.

You can also specify any additional default command line arguments for RV in the *RV Preferences* dialog.

If you have a RAID or other fast storage device you may want to configure the RV/Nuke integration to use a directory on this device as the base for all Session directories (see below). If so set the "Default Session Dir Base" preference accordingly.

### Rv Project Settings

There are several settings that the integration uses that may be different for different Nuke projects. Once you have a script loaded, select the *RV/Project Settings…* menu item, and then the *RV* tab of the Project Settings.

The table below lists all the RV Project Settings, with explanations, but the most important is the “Session Directory.” This directory is where all media, script versions, and other information is stored for this Nuke script/project. It **must** be unique for each project.

Session Directory

The root directory for all media, scripts and other information related to this project. It will be created if it does not exist. Since media will be stored under this directory, you may want to put it on a device with fast IO. This name **must** be uniqe across all projects.

You can set "Default Session Dir Base" in the RV Preferences (see above) so that by default all Session directories are created on your fast IO device.

Render File Format

The format of all media files created by rendering and checkpointing.

Nuke Node Selection → RV Current View

If this box is checked, every time you select a node in nuke, if RV is connected, the current RV view node will be set to the corresponding view. This lets you quickly view or play media, either input media associated with a Read node, or rendered media associated with any node that has been checkpointed or rendered.

Nuke Frame → RV Frame

If this box is checked, frame changes in Nuke will force the corresponding frame change in RV.

Nuke Read Node Changes → RV Sources

If this box is checked, the total set of Read nodes in the project will be dynamically synced to RV. That is, for every Read node in the project, there will be a corresponding Source in RV with the same media, available for playback on demand. Adding or Deleteing a Read node in Nuke will trigger the corresponding action in RV. Changes to Read node file path, frame range, and color space will also be reflected in RV.

### Quick Start Summary

You must set the RV executable path using the *RV/Preferences..* menu item before you use RV with Nuke at all, and whenever you start work on a new project/script, use *RV/Project Settings…* to make sure that the *Session Directory* is set to something reasonable before you start RV from that script for the first time. See above for details.

### Open RV Toolbar

Note that all the items on the RV menu are also available on the RV toolbar, which you can find in the Panes submenu.

## Read/Write Nodes


Once you’ve set the RV path and Session Dir as described above, and have an interesting Nuke script loaded, try starting up RV with the *RV/Start RV* menu item. Assuming you have the *Sync Read Changes* setting active, as soon as RV starts you should see all the Read nodes in the script reflected as media Sources in RV.

If you don’t see the Session Manager, try hitting the *x* to bring it up. In the Session Manager, You’ll see a Folder called “Read Nodes” with a Source for each Read node in the script. Each source is labeled with the name of the corresponding Read node, and a timestamp for when it was last modified.

> **Note:** The Session Manager behavior at RV start-up can be set to "aways shown", "always hidden" or "remember previous state" using the "wrench" menu on the Session Manager.

You can double-click on each Source to play just that one, or on the “Read Nodes” folder to see them all.

Back in Nuke, note that if you edit the Path, Frame Range, or Color Space attributes of a Read node, the changes are reflected in the corresponding Source in RV.

If the *Sync Selection* setting is active, as you select various Read nodes in Nuke, the RV current view switches to the corresponding Source.

Also, if the *Sync Frame* setting is active, frame changes in the Nuke viewer will be reflected in RV.

Note that if you don’t want all Read Nodes to be synced automatically, you can still sync some (or all) of them when you want to with the appropriate items on the *RV* menu.

Pretty much all the above applies to Write nodes as well.

## Checkpoints and Renders


As with Read nodes, Checkpoints and Renders are representations in RV of particular nodes in Nuke. So the Frame and Selection syncing described in the Read Nodes section applies to Checkpoints and Renders as well.

Unlike Read nodes, the media associated with Checkpoints and Renders are generated from the Nuke script and so reflect the state of the script at the time of rendering.

### Checkpoints

The point of a Checkpoint is to to visually label a particular point in your projects development, so that you can easily return to that point if you want to. When you’ve made some changes in your script, and reach a point where you want to go in another direction, or try something out, or work on a different aspect of the project, that’s a good time to “bookmark” your work with a Checkpoint.

To make a Checkpoint, select a node that visually reflects the state of the script and select *RV/Create Checkpoint* . You’ll see a new Source appear in RV, in a Folder named for the node you selected, with a single rendered frame from that node.

As you work on a particular aspect of your project, you may want to make many Checkpoints of a particular node, so that you can easily compare the visual effect of different parameter settings. They’ll all be collected in a single folder in the Session Manager, and as with Read nodes, you can double click on a single one to view it, or double click on the folder itself to see them all.

### Rendering

A Render is similar to a Checkpoint, but involves rendering a sequence of frames, instead of just one. To render, select the node of interest, then select *RV/Render to RV* . You’ll get a dialog with some parameters:

Output Node

The name of the node to be rendered.

Use Selected

If checked, the output node will always be equal to whatever node is selected when the dialog is shown. If unchecked, the output node will “stick” and not be affected by the selection.

First Frame

The first frame in the sequence to be rendered.

Last Frame

The last frame in the sequence to be rendered.

Since Renders can occupy significant disk space, successive renders of the same node overwrite any pre-existing render. But each render also automatically generates a single-frame Checkpoint of the same Nuke state. Also, deleting a Render or Checkpoint in the Session Manager (with the Trash Can button), also removes the corresponding media from disk.

During a Render, RV updates dynamically to show you all the frames rendered so far. If the render is canceled, you still see in RV any frames that completed before the cancel. RV Sources from renders go into the same Folder as Checkpoints from the same node.

### Full Checkpoints

A Full Checkpoint is just like a regular checkpoint except that an entire sequence of frames is saved. To create a Full Checkpoint, select a Render in the RV Session Manager and then select *Create Full Checkpoint* from the *Nuke* menu in RV.

## Working with Media in Open RV

Relevant here is the chapter on the [Session Manager](../rv-manuals/rv-user-manual/rv-user-manual-chapter-five.md) and the section on [navigation](../rv-manuals/rv-user-manual/rv-user-manual-chapter-five.md#54-navigating-between-views)

### Folders

The Nuke integration makes use of Folders to organize your media. You’ll have a folder for all your Read nodes, a folder of checkpoints and renders for each rendered node, and a catch-all folder called “Other” to collect the rest. All folders are viewable and make for a handy “browsing” interface.

### Comparisons

You can easily Compare two or more renders or checkpoints (or any views, actually). Just select the views of interest in the Session Managerand select on the comparison items on RV’s *Nuke* menu: *Nuke/Wipe Selected Views* or *Nuke/Tile Selected Views* .

## Modifying the Nuke Project from Open RV


### Restoring Checkpoints

Any Checkpoint (or Render) can provide a source from which the Nuke project can be restored to the state it was in when the Checkpoint’s media was rendered. To restore a Checkpoint, select it in the RV Session Manager, and choose *Nuke/Restore Checkpoint* . After a confirmation dialog, the Nuke script will be restored.

The navigation techniques referenced above combine with checkpoint restoration to produce some nice workflows (I think). For example:

1.  After lots of rendering and checkpointing of node *FinalMerge* , double-click on the *Renders of FinalMege* folder to see a layout of all the checkpoints and renders.
    
2.  Bring up the Image Info widget to mouse around and see the names and timestamps of all the views in the layout.
    
3.  Double click on one if the tiles to examine that checkpoint more closely.
    
4.  Decide to restore this checkpoint, it’s alread selected, so just hit *Nuke/Restore Checkpoint*
    

Also note that the Restore operation is undo-able, from the Nuke *Edit* menu.

### Adding Read Nodes

Of course you can still view media that’s unconnected to the Nuke project in a connected RV. So you can for example browse an element library. Once you have media that you’d like to include in your project, just select the Sources in the Session Manager and choose *Nuke/Create Nuke Read Node* . The corresponding Read node will be created in Nuke. Actually you can create any number at once by just selecting however many you want.
