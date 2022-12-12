# Chapter 1 - Introduction

### 1.1 Overview


RV and its companion tools, RVIO and RVLS have been created to support digital artists, directors, supervisors, and production crews who need reliable, flexible, high-performance tools to review image sequences, movie files, and audio. RV is clean and simple in appearance and has been designed to let users load, play, inspect, navigate and edit image sequences and audio as simply and directly as possible. RV's advanced features do not clutter its appearance but are available through a rich command-line interface, extensive hot keys and key-chords, and smart drag/drop targets. RV can be extensively customized for integration into proprietary pipelines. The RV Reference Manual has information about RV customization.

This chapter provides quick-start guides to RV and RVIO. If you already have successfully installed RV, and want to get going right away, this chapter will show you enough to get started.

### 1.2 Getting Started With Open RV


#### 1.2.1 Loading Media and Saving Sessions

There are four basic ways to load media into RV,

1.  Command-line,
2.  File open dialogs,
3.  Drag/Drop, and
4.  rvlink: protocol URL

RV can load individual files or multiple files (i.e. a sequence) and it can also read directories and figure out the sequences they contain; you can pass RV a directory on the command-line or drag and drop a folder onto RV. RV's ability to read directories can be particularly useful. If your shots are stored as one take per directory you can get in the habit of just dropping directories into RV or loading them on the command line. Or you can quickly load multiple sequences or movies that are stored in a single directory.

Some simple RV command line examples are:

```
shell> rv foo.mov
shell> rv [ foo.#.exr foo.aiff ]
shell> rv foo_dir/
shell> rv .
```

and of course:

```
shell> rv -help
```

The output of the -help flag is reproduced in this manual in the chapter on command-line usage, Chapter [3](rv-user-manual-chapter-three.md#3-command-line-usage) .

RV sessions can be saved out as .rv files using the File->Save menus. Saved sessions contain the default views, user-defined views, color setup, compositing setup, and other settings. This is useful for reloading and sharing sessions, and also for setting up image conversion, compositing, or editing operations to be processed by RVIO.

#### 1.2.2 Caching

If your image sequences are too large to play back at speed directly from disk, you can cache them into system memory using RV's *region cache* . If you are playing compressed movies like large H.264 QuickTime movies, you can use RV's *lookahead cache* to smooth out playback without having to cache the entire movie. If your IO subsystems can provide the bandwidth, RV can be used to stream large uncompressed images from disk. You can set the RV cache options from the Tools menu, using the hot keys \`\`Shift+C” and \`\`Ctrl-L” (\`\`Command+L” on Mac) for the region cache and lookahead cache respectively, or from the command line using \`\`-c” or \`\`-l” flags. Also see the Caching tab of the Preferences dialog.

#### 1.2.3 Sources and Layers

RV gives you the option to load media (image sequences or audio) as a *Source* or a *Layer* . A source is a new sequence or movie that gets added to the end of the default sequence of the RV session. Adding sources is the simplest way to build an edit in RV. Layers are the way that RV associates related media, e.g. an audio clip that goes with an EXR sequence can be added as a layer so that it plays back along with the sequence. Layers make it very simple to string together sequences with associated audio clips–each movie or image sequence can be added as source with a corresponding audio clip added as a layer (see soundfile commandline example above). RV's stereoscopic display features can interpret the first two image layers in a source as left and right views.

#### 1.2.4 Open RV Views

RV provides three default views, and the ability to make views of your own. The three that all sessions have are the Default Sequence, which shows you all your sources in order, the Default Stack, which shows you all your sources stacked on top of one another, and the Default Layout, which has all the sources arranged in a grid (or a column, row, or any other custom layout of your own design). In addition to the default views, you can create any number of Sources, Sequences, Stacks, and Layouts of your own. See [5](rv-user-manual-chapter-five.md#51-rv-session) for information about the process of creating and managing your own views.
 
#### 1.2.5 Marking and Navigating

RV's timeline (hit TAB or F2 to bring it up) can be *marked* to make it easy to navigate around an RV session. RV can mark sequence boundaries automatically, but you can also use the \`\`m” key to place marks anywhere on the timeline. Once a session is marked you can use hot keys to quickly navigate the timeline, e.g. \`\`control+right arrow” (\`\`command+right arrow'' on Mac) will set the in/out points to the next pair of marks so you can loop over that part of the timeline. If no marks are set, many of these navigation options interpret the boundaries between sources as “virtual marks”, so that even without marking you can easily step from one source to the next, etc.

#### 1.2.6 Color

RV provides fine grained control over color management. Subsequent sections of this manual describe the RV color pipeline and options with a fair amount of technical detail. RV supports file LUTs and CDLs per source and an overall display LUT as well as a completely customizable 'source setup' function (described in the RV Reference Manual). For basic operation, however, you may find that the built in hardware conversions can do everything you need. A common example is playing a QuickTime movie that has baked-in sRGB together with an EXR sequence stored as linear floating point. RV can bring the QuickTime into linear space using the menu command Color->sRGB and then the whole session can be displayed to the monitor using the menu command View->sRGB.

LUTs can be loaded into RV by dragging and dropping them onto the RV window, through the File->Import menus and on the command line using the -flut, -llut, -dlut, and -pclut options. Similarly CDLs can be loaded into RV by dragging and dropping them onto the RV window, through the File->Import menus and on the command line using the -fcdl and -lcdl options.

#### 1.2.7 Menus, Help and Hot Keys

Help->Show Current Bindings will print out all of RV's current key bindings to the shell or console (these are also included in chapter X of this manual). RV's menus can be reached through the menu bar or by using the right mouse button. Menus items with hot keys will display the hot key on the right side of the menu item. Some hot keys worth learning right away are:

*   Space - Toggle playback
*   Tab or F2 - Toggle Show Timeline
*   'i' - Toggle Show Info Widget
*   '\`' - (back-tick) Toggle Full Screen
*   F1 - Toggle Show Menu
*   Shift + Left Click - Open Pixel Inspector at pointer
*   “q” to quit RV (or close the current session)


#### 1.2.8 Parameter Editing and Virtual Sliders

Many settings in RV, like exposure, volume, or frame rate, can be changed quickly using Parameter Edit Mode. This mode lets you use virtual sliders, the mouse wheel or the keyboard to edit RV parameters. Hot keys and Parameter Editing Mode allow artists to easily and rapidly interact with images in RV. It is worth a little practice to get comfortable using these tools. For example, to adjust the exposure setting of a sequence you can use any of the following techniques:

1.  Hit the 'e' key to enter exposure editing mode Then:
2.  Click and drag left or right to vary the exposure, and then release the mouse button to leave the mode,
3.  OR: Roll the mouse wheel to vary the exposure and then hit return to leave the mode.
4.  OR: Hit return, type the new exposure value at the prompt, and hit return again (typing '.' or any digit also starts this text-entry mode)
5.  OR: Use the '+' and '-' keys to vary the exposure and then hit return to leave the mode.

Some advanced usage:

*   Use the 'r' 'g' 'b' keys to edit individual color channels. ('c' to return to editing all 3 channels.) Parameters that can be “unganged” in this way will display a 3-color glyph in the display feedback when you start editing.
*   Hit the 'l' to lock (or unlock) slider mode, so that you can repeatedly set the same parameter ('ESC' to exit).
*   The 'DEL' or 'BackSpace' key will reset the parameter to it's default value.
*   When multiple Sources are visible, as in a Layout view, parameter sliders will affect all Sources. Or you can use 's' to select only the source under the pointer for editing.

Some parameters in RV don't use virtual sliders; you can edit these directly by entering the new value, e.g. to change the playback frames per second:

1.  Hit Shift+F
2.  Type in the new frame rate at the prompt and hit return

Some other useful parameters and their hot keys:

*   Gamma - 'y'
*   Hue - 'h'
*   Contrast - 'k'
*   Audio Volume - ctrl-'v' (command-'v' on Mac)

#### 1.2.9 Preferences and Command Line Parameters

RV's Preferences can be opened with the RV->Preferences menu item. These are worth exploring in some detail. They give you fine control over how RV loads and displays images, handles color, manages the cache, handles audio, etc. RV's preferences map to RV's command line options, so almost any option available at the command line can be set to a preferred default value in the Preferences. RV also has a -noPrefs command line flag so that you can temporarily ignore the preferences, and a -resetPrefs flag that will reset all preferences to their default values. The quickest way to take a look at all of RV's command line options is:

```
shell> rv -help
```

### 1.2.10 Customizing Open RV

RV is built to be customized. For many users, this may be completely ignored or be limited to sharing startup scripts and packages created by other users. A package is a collection of script code (Mu or Python) and interface elements which can be automatically loaded into RV. A package can be installed site wide, per-show, or per-user and a command line tool (rvpkg) is included for package administration tasks.

RV Packages are discussed in Chapter [10](rv-user-manual-chapter-ten.md#10-packages) .

Customization is discussed in detail in the RV Reference Manual. The RV command API technical documentation can be browsed from Help->Mu Command API Browser.

### 1.3 Getting Started with RVIO

#### 1.3.1 Converting Sequences and Audio

RVIO is a powerful pipeline tool. Like RV, the basic operation of RVIO is very simple, but advanced (and complex) operations are possible. RVIO can be used with a command line very similar to RV's, with additional arguments for specifying the output. Any number of sources and layers can be given to RVIO using the same syntax as you would use for RV. Some basic RVIO command line examples are:

```
shell> rvio foo.exr -o foo.mov
shell> rvio [ foo.#.exr foo.aiff ] -o foo.mov
shell> rvio [ foo_right.#.exr  foo_left.#.exr foo.aiff ] \
       -outstereo -o foo.mov
```

And of course:

```
shell> rvio -help
```

RVIO usage is more fully described in Chapter [16](rv-user-manual-chapter-sixteen.md#16-rvio) .

#### 1.3.2 Processing Open RV Session Files

RVIO can also take RV session files (.rv files) as input. RV session files can contain composites, color corrections, LUTs, CDLs, edits, and other information that might be easier to specify interactively in RV than by using the command line. RV session files can be saved from RV and then processed with RVIO. For example

```
shell> rvio foo.rv -o foo_out.#.exr
```

When rvio operates on a session file, any of the Views defined in the session file can be selected to provide rvio's output, so a single session could generate any number of different output sequences or movies, depending on which of the session's views you choose.

#### 1.3.3 Slates, Mattes, Watermarks, etc.

RVIO uses Mu scripts to create slates, frame burn-in and other operations that are useful for generating dailies, client reviews and other outputs. These scripts are usable as is, but they can also be modified or replaced by users. Some examples are:

```
shell> rvio foo.#.exr -overlay watermark "For Client Review" 0.5 \
       -o foo.mov
shell> rvio foo.#.exr -leader simpleslate \
       "Tweak Films" \
       "Artist=Jane Doe" \
       "Shot=SC101_vfx_01" \
       "Notes=Lighter/Darker" \
       -o foo.mov
```