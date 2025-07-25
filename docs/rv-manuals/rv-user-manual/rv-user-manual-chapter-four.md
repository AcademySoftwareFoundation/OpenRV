# Chapter 4 - User Interface

The goal of RV's user interface is to be minimal in appearance, but complete in function. By default, RV starts with no visible interface other than a menu bar and the timeline, and even these can be turned off from the command line or preferences. While its appearance is minimal, its interaction is not: almost every key on the keyboard does something and it's possible to use key-chords and prefix-keys to extend this further.

Emacs users will find this feature familiar. RV can have prefix-keys that when pressed remap the entire keyboard or mouse bindings or both.

The main menu and pop-up menus allow access to most functions and provide hot keys where available.

RV makes one window per session. Each window has two main components: the viewing area—where images and movies are shown— and the menu bar. On macOS, the menu bar will appear at the top of the screen (like most native macOS applications). On Linux and Windows, each RV window has its own attached menu bar.

A single RV process can control multiple independent sessions on all platforms. On the Mac and Windows, it is common that there be only a single instance of RV running. On Linux it is common to have multiple separate RV processes running.

Many of the tools that RV provides are heads up widgets. The widgets live in the image display are or are connected to the image itself. Aside from uniformity across platforms, the reason we have opted for this style of interface was primarily to make RV function well when in full screen mode.

![4_linux_snapshot.jpg](../../images/rv-user-manual-4-rv-cxx-rv-linux-snapshot-03.jpg)   ![5_osx_snapshot.jpg](../../images/rv-user-manual-5-rv-cxx-rv-osx-snapshot-04.jpg)

Table 4.1: RV on Linux and on macOS.

### 4.1 Feedback

RV provides feedback about its current state near the top left corner of the window.

![6_rv_rv-cxx98-release_feedback.jpg](../../images/rv-user-manual-6-rv-cxx98-release-feedback-05.jpg)  

Figure 4.1: Feedback widget indicating full-color display

### 4.2 Main Window Toolbars

The main RV window contains two toolbars that are visible by default:

- **Upper toolbar**: Controls view display, viewing options, and display device settings
- **Lower toolbar**: Controls playback, tool buttons, and audio functions

![7_toolbars_legend.png](../../images/rv-user-manual-7-rv-cxx-toolbars-legend-06.png)  

Figure 4.2: Toolbar Controls

#### Lower Toolbar Layout

The lower toolbar contains three sections (left to right):

1. **Tool launch buttons**: Toggle RV's main UI components (session manager, heads-up timeline)
2. **Annotations display controls**: Ghost and Hold control the display of annotations
3. **Play controls**: Control playback in the current view (similar to heads-up play controls)
4. **Audio/loop mode**: Determine timeline end behavior and control volume/mute

##### About Hold and Ghost

The Ghost and Hold display controls help manage how annotations appear during playback. These controls are useful during a Live Review session by helping reviewers follow feedback and understand the context of comments.

**Ghost annotations**: Provides temporal context by showing annotations from nearby frames. This helps reviewers understand the progression of feedback and see upcoming annotations, maintaining context during continuous playback.

- **Previous annotations**: The previous five annotations appear in red, with decreasing opacity as they get further from the current frame.
- **Upcoming annotations**: The next five annotations appear in green with decreasing opacity, with decreasing opacity as they get further from the current frame.

**Hold annotations**: Keeps the current annotation visible during playback, holding it on screen until the next annotation. Important feedback remains visible even during fast playback, preventing viewers from missing critical notes.

#### Additional Access Points

- Upper toolbar functions (frame content, display device settings, channel view, stereo mode) are also available in the **View** menu
- Full-screen toggle is available in the **Window** menu  
- Toolbar visibility can be toggled in the **Tools** menu

For detailed information about these settings, see Chapter [7](rv-user-manual-chapter-seven.md#7-how-a-pixel-gets-from-a-file-to-the-screen).

### 4.3 Loading Images, Sequences, Movies and Audio


#### 4.3.1 Using the File Browser

There are two options for loading images, sequences, and movies via the file browser: you can add to the existing session by choosing File → Open (or File → Open into Layer) or you can open images in a new window by choosing File → Open In New Session.

In the file browser, you may choose multiple files. RV tries to detect image sequences from the names of image files (movie files are treated as individual sequences). If RV detects a pattern, it will create an image sequence for each unique pattern. If no pattern is found, each individual image will be its own sequence.

Audio files can be loaded into RV using the file browser. To associate an audio file with its corresponding image sequence or movie open the audio file as a layer using File → Open into Layer. The first sequence in the layer will determine the overall length of the source, e.g. a longer audio file loaded as a layer after a sequence will be truncated to the duration of the sequence. Any number of Audio files can be added as layers to the same source and they will be mixed together on playback.

The file browser has three file display modes: column view, file details view, and media details view. Sequences of images appear as virtual directories in the file browser: you can select the entire sequence or individual files if you open the sequence up. **Note:** You can multi-select in File Details and Media Details, but not in Column View. In general the File Details view will be the fastest.

![8_rv_rv-cxx98-release_grabFile.png](../../images/rv-user-manual-8-rv-cxx98-release-grabFile-07.png)  

Figure 4.3: File Browser Show File Details

Favorite locations can be remembered by dragging directories from the main part of the file browser to the side bar on the left side of the dialog box. Recent items and places can be found under the path combo box. You can configure the way the browser uses icons from the preferences drop down menu on the upper right of the browser window.

#### 4.3.2 Dragging and Dropping

On all platforms, you can drag and drop file and folder icons into the RV window. RV will correctly interpret sequences that are dropped (either as multiple files or inside of a directory folder that is dropped). LUT and CDL files can also be dropped.

RV uses smart drop targets to give you control over how files are loaded into RV. You can drop files as a *source* or as a *layer* . As you drag the icons over the RV window, the drop targets will appear. Just drop onto the appropriate one.

On Linux RV should be compatible with the KDE and Gnome desktops. It is possible with these desktops (or any that supports the XDnD protocol) to drag file icons onto an RV window.

If multiple icons are dropped onto RV at the same time, the order in which the sequences are loaded is undefined.

To associate an audio file with an image sequence or movie, drop the audio file as a layer, rather than as a source.

### 4.4 Examining an Image


RV normalizes image geometry to fit into its viewing window. If you load two files containing the same image but at different resolutions, RV will show you the images with the same apparent \`\`size''. So, for example, if these images are viewed as a sequence — one after another — the smaller of the two images will be scaled to fit the larger. Of course, if you zoom in on a high-resolution image, you will see detail compared to a lower-resolution image. When necessary you can view the image scaled so that one image pixel is mapped to each display pixel.

On startup, RV will attempt to size the window to map each pixel to a display pixel, but if that is not possible, it will settle on a smaller size that fits. You can always set the scale to 1:1 with the '1' hotkey, and if it is possible to resize the window to contain the entire image at 1:1 scaling, you can do so via the Window → Fit or Window → Center Fit menu items.

#### 4.4.1 Panning, Zooming, and Rotating

You can manipulate the Pan and Zoom of the image using the mouse or the row of number keys (or the keypad on an extended keyboard if numlock is on). By holding down the control key (apple key on a mac) and the left mouse button you can zoom the image in or out by moving to the left and right. By holding down the alt key (or option key on a mac) you can pan the image in any direction. If you are accustomed to Maya camera bindings, you can use those as well.

Rotating the image is accessible from the Image → Rotation menu. By selecting Image → Rotation → Arbitrary it's possible to use the mouse to scrub the rotation as a parameter.

To frame the image — automatically pan and zoom it to fit the current window dimensions — hit the 'f' key. If the image has a rotation, it will remain rotated.

To precisely scale the image you can use the menu Image → Scale to apply one of the preset scalings. Selected 1:1 will draw one image pixel at every display pixel. 2:1 will draw 1 image pixel as 4 display pixels, etc.

#### 4.4.2 Inspecting Pixel Values

The pixel inspector widget can be accessed from the Toools menu or by holding down Shift and clicking the left mouse button. The inspector will appear showing you the source pixel value at that point on the image (see Figure [cap:Color-Inspector](#color-inspector) ). If you drag the mouse around over the image while holding down the shift key the inspector widget will also show you an average value (see Figure [cap:Average-Color](#average-color) ). You can move the widget by clicking on it and dragging.

To remove inspector widget from the view either movie the mouse to the top left corner of the widget and click on the close button that appears or toggle the display with the Tools menu item or hot key.

The inspector widget is locked to the image. If you pan, zoom, flip, flop, or rotate the image, the inspector will continue to point to the last pixel read.

If you play a sequence of images with the inspector active, it will average the pixel values over time. If you drag the inspector while playback occurs it will average over time and space.

RV shows either the source pixel values or the final rendered values. The source value represents the value of the pixel just after it is read from the file. No transforms have been done on the pixel value at that point. You can see the final pixel color (the value after rendering) by changing the pixel view to the final pixel value from the right-click popup menu.

The value is normalized if the image is stored as non-floating point — so values in these types of images will be restricted to the [0,1] range. Floating point images pass the value through unchanged so pixels can take values below zero or above one. Table [7.3](rv-user-manual-chapter-seven.md#characteristics-of-channel-data-types) shows the range of each of the channel data types.

![11_rv_rv-cxx98-release_inspector.jpg](../../images/rv-user-manual-11-rv-cxx98-release-inspector-010.jpg)

Figure 4.6: Color inspector <a id="color-inspector"></a>

![12_rv_rv-cxx98-release_average.jpg](../../images/rv-user-manual-12-rv-cxx98-release-average-11.jpg)  

Figure 4.7: Average Color <a id="average-color"></a>

From the right-click popup menu it's possible to view the pixel values as normalized to the [0,1] range or as 8, 16, 10, or 12 bit integer values.

#### 4.4.3 Comparing Images with Wipes

Wipes allow you to compare two or more images or sequences when viewing a stack. Load the images or sequences that you wish to compare into RV as sources (not as layers). Put RV into stack mode or create a stack view from the Session Manager, and enable wipes from the Tools menu or with the “F6” hot key. Now you can grab on the edges of the top image and wipe them back to reveal the image below. You can grab any edge or corner, and you can move the entire wipe around by grabbing it in the exact center. Also, by clicking on the icon that appears at the center or corner of a wipe or via the Wipes menu, you can enable the wipe information mode, that will indicate which edge you are about to grab.

Wipes can be used with any number of sources. The stack order can be cycled using the “(“ and “)” keys.

#### 4.4.4 Parameter Edit Mode and Virtual Sliders

RV has a special UI mode for editing the parameters such as color corrections, volume, and image rotation. For example, hitting the \`\`y'' key enters gamma edit mode. When editing parameters, the mouse and keyboard are bound to a different set of functions. On exiting the editing mode, the mouse and keyboard revert to the usual bindings. (See Table [4.2](#parameter-edit-mode-key-and-mouse-bindings) )

To edit the parameter value using the mouse you can either scrub (like a virtual slider) or use the wheel. If you want to eyeball it, hold the left mouse down and scrub left and right. By default, when you release the button, the edit mode will be finished, so if you want to make further changes you need to re-enter the edit mode. If you want to make many changes to the same parameter, you can “lock” the mode with the 'l' key. The scroll wheel increments and decrements the parameter value by a predefined amount. Unlike scrubbing with the left mouse button, the scroll wheel will not exit the edit mode. When multiple Sources are visible, as in a Layout view, parameter sliders will affect all Sources. Or you can use 's' to select only the source under the pointer for editing. You can exit the edit mode by hitting the escape key or space bar (or most other keys).

To change the parameter value using the keyboard, hit the Enter (or Return) key; RV will prompt you for the value. For interactive changes from the keyboard, use the \`\`+'' and \`\`-'' keys (with or without shift held down). The parameter is incremented and decremented. To end the keyboard interactive edit, hit the Escape or Spacebar keys.

| Key/Mouse Sequence | Action                             |
| ---------------------- | -------------------------------------- |
| Mouse Button #1 Drag   | Scrub parameter                        |
| Mouse Button #1 Up     | Finish parameter edit                  |
| Wheel                  | Increment or decrement parameter       |
| Enter                  | Enter parameter numerically            |
| 0 through 9            | Enter parameter numerically            |
| ESC                    | Cancel parameter edit mode             |
| \+ or =                | Increment parameter value              |
| - or _               | Decrement parameter value              |
| BACKSPACE or DEL       | Reset parameter value to default       |
| r or g or b            | Edit single channel of color parameter |
| c                      | Edit all channels of color parameter   |
| l                      | Lock or unlock editing mode            |
| s                      | Select single Source for editing       |

Table 4.2: Parameter Edit Mode Key and Mouse Bindings <a id="parameter-edit-mode-key-and-mouse-bindings"></a>

#### 4.4.5 Image Filtering

When image pixels are scaled to be larger or smaller than display pixels, resampling occurs. When the image is scaled (zoomed) RV provides two resampling methods (filters): nearest neighbor and linear interpolation (the default).

You can see the effects of the resampling filters by making the scale greater than 1:1. This can be done with any of the hot keys \`\`2'' through \`\`8'' or by zooming the image interactively. When the image pixels are large enough, you can switch the sampling method via View → Linear Filter or by hitting the \`\`n'' key. Figure [4.3](#nearest-neighbor-and-linear-interpolation-filtering) shows an example of an image displayed with nearest neighbor and linear filtering.

![14_e_nearest_filter.jpg](../../images/rv-user-manual-14-rv-cx-e-nearest-filter-13.jpg)   ![15_se_linear_filter.jpg](../../images/rv-user-manual-15-rv-cx-se-linear-filter-14.jpg)

Table 4.3: Nearest Neighbor and Linear Interpolation Filtering. Nearest neighbor filtering makes pixels into blocks (helpful in trying to determine an exact pixel value). <a id="nearest-neighbor-and-linear-interpolation-filtering"></a>

##### Digression on Resampling

It's important to know about image filtering because of the way in which RV uses the graphics hardware. When an image is resampled—as it is when zoomed in—and the resampling method produces interpolated pixel values, correct results are really only obtained if the image is in linear space. Because of the way in which the graphics card operates, image resizing occurs before operations on color. This sequence can lead to odd results with non-linear space images if the linear filter is used (e.g., cineon files).

If you want to put a positive spin on it you could say you're using a non-linear resampling method on purpose. The results are only incorrect if you meant to do something else!

There are two solutions to the problem: use the nearest neighbor filter or convert the image to linear space before it goes to the graphics card. The only downside with the second method is that the transform must happen in software which is usually not as fast. Of course this only applies to images that are not already in linear space.

Why does RV default to the linear filter? Most of the time, images and movies come from file formats that store pixel values in linear (scene-referred) space so this default is not an issue. It also looks better.

The important thing is to be aware of the issue.

##### Floating Point Images

If RV is displaying floating point data directly, linear filtering may not occur even though it is enabled. This is a limitation of some graphics cards that will probably be remedied (via driver update or new hardware) in the near future. In this case You can make the filter work by disallowing floating point values via Image → Color Resolution → Allow Floating Point. Many graphics can do filtering on 16 bit floating point images but cannot do filtering on 32 bit floating point images. RV automatically detects the cards capabilities and will turn off filtering for images if necessary.

Figure [4.4](#floating-point-filter) shows an example of a floating point image with linear filtering enabled versus equivalent 8-bit images.

![16_ase_float_linear.jpg](../../images/rv-user-manual-16-rv-cx-ase-float-linear-15.jpg)   ![17_ease_8bit_linear.jpg](../../images/rv-user-manual-17-rv-cx-ease-8bit-linear-16.jpg) ![18_ase_8bit_nearest.jpg](../../images/rv-user-manual-18-rv-cx-ase-8bit-nearest-17.jpg)

Table 4.4: Floating point linear, 8 bit linear, and 8 bit nearest neighbor filtering.

Graphics hardware does not always correctly apply linear filtering to floating point images. Filtering can dramatically change the appearance of certain types of images. In this case, the image is composed of dense lines and is zoomed out (scaled down). <a id="floating-point-filter"></a>

#### 4.4.6 Big Images

RV can display any size image as long as it can fit into your your computer's memory. When an image is larger than the graphics card can handle, RV will tile the image display. This makes it possible to send all the pixels of the image to the card for display. The downside is that all of the pixels are sent to the display even though you probably can't see them all. However, if you zoom in (for example hit \`\`1'' for 1:1 scale) when a large image is loaded, RV will only draw pixels that are visible.

One of the constraints that determines how big an image can be before RV will tile it is the amount of available memory in your graphics card and limitations of the graphics card driver. On most systems, up to 2k by 2k images can be displayed without tiling (as long as the image has 8-bit integer channels). In some cases (newer cards) the limit is 4k by 4k. However, there are other factors that may reduce the limit.

If your window system uses the graphics card (like macOS or Linux with the X.org X server) or there are other graphics-intensive applications running, the amount of available memory may be dependent on these processes as well. Alternately, because RV wants to use as much graphics memory as it can, RV may cause graphics resource depletion that affects other running applications that should have higher priority. Because of this, RV has the capability to limit its graphics memory usage. You can specify this in RV's Preferences by editing the *Maximum VRAM Usage Per Tile* or on the command line with the -vram option.

Over time, these problems will go away as drivers and operating systems become smarter about graphics resource allocation.

If you reduce the VRAM usage, RV will tile images of smaller size. For sequences, this may affect playback speed since tiling is slightly less efficient than not tiling. Tiling also affects interactive speed on single images; if tiling is not on, RV can keep all of the image pixels on the graphics card. If tiling is on, RV has to send the pixels every time it redraws the image.

You can determine if RV is tiling the image by looking the image info widget under Tools → Image Info. If tiling is on there will be an entry called \`\`DisplayTiling'' showing the number of tiles in X and Y.

#### 4.4.7 Image Information

The image information widget, can be shown or hidden via the Tools → Image Info menu item or using the hot key: \`\`i''. You can move the widget by clicking and dragging. The widget shows the geometry and data type of the image as well as associated meta-data (attributes in the file). Figure [4.9](#image-information-widget) shows an example of the information widget.

![19_e_infoWidgetShot.png](../../images/rv-user-manual-19-rv-cx-e-infoWidgetShot-18.png)  

Figure 4.9: Image Information Widget. <a id="image-information-widget"></a>

Channel map information—the current mapping of file channels to display channels—is displayed by the info widget as well as the names of channels available in the image file; this display is especially useful when viewing an image with non-RGBA channels.

If the image is part of a sequence or movie the widget will show any relevant data about both the current image as well as the sequence it is a part of. For movie files, the codecs used to compress the movie are also displayed. If the movie file has associated audio data, information about that will also appear.

To remove the image information widget from the view either move the mouse to the top left corner of the widget and click on the close button that appears or toggle the display with the Tools menu item or hot key ('i').

### 4.5 Playing Image Sequences, Movie Files, and Audio Files


RV can play multiple images, image sequences and movie files as well as associated audio files. Play controls are available via the menus, keyboard, and mouse. Timing information and navigation is provided by the timeline widget which can be toggled via the Tools → Timeline menu item or by hitting the TAB key.

#### 4.5.1 Timeline

![20_timeline_labelled.png](../../images/rv-user-manual-20-rv-cx-timeline-labelled-19.png)  

Figure 4.10: Timeline With Labelled Parts

This timeline shows in and out points, frame count between in and out points, total frames, target fps and current fps. In addition, if there are frame marks, these will appear on the timeline as seen in Figure [4.5.5](#timline-with-marks) .

The current frame appears as a number positioned relative to the start frame of the session. If in and out points are set, the relative frame number will appear at the left side of the timeline — the total number of frames between the in and out points is displayed below the relative frame number.

Tip: To change the start frame of a Version (with or without Slate): Toggle **Movie Has Slate** (or **Frames have Slate**) to on, and then set the First Frame and Last Frame Fields on the Version. If your Movie (or Frames) are 201 to 299 and 201 is the slate, you need to set First Frame to 202.

By clicking anywhere on the timeline, the frame will change. Clicking and dragging will scrub frames, as will rolling the mouse-wheel. Also note that you can shift-click drag to set an in/out range.

The in/out range can also be manipulated with the mouse. You can grab and drag either end, or grab in the middle to drag the whole range.

There are two FPS indicators on the timeline. The first indicates the target FPS, the second the actual measured playback FPS.

| | |
| -------------------------- | ---------------------------------------------------------------------------------- |
| [                          | Set in point                                                                       |
| ]                          | Set out point                                                                      |
| \                          | Clear in/out points                                                                |
| right-arrow                | Step one frame to the right.                                                       |
| left-arrow                 | Step on frame to the left.                                                         |
| alt-right-arrow            | Move current frame to next mark (or source boundary, if there are no marks)        |
| alt-left-arrow             | Move current frame to previous mark (or source boundary, if there are no marks)    |
| ctrl(mac: cmd)-left-arrow  | Set in/out to next pair of marks (or source boundaries, if there are no marks)     |
| ctrl(mac: cmd)-right-arrow | Set in/out to previous pair of marks (or source boundaries, if there are no marks) |
| down-arrow, spacebar       | Toggle playback                                                                    |
| up-arrow                   | Reverse play direction                                                             |

Table 4.5: Useful Timeline Hotkeys <a id="useful-timeline-hotkeys"></a>

![21_timeline_reddot.png](../../images/rv-user-manual-21-rv-cx-timeline-reddot-20.png)  

**Note:** A red dot with a number indicates how many frames RV has lost since the last screen refresh.

#### 4.5.2 Timeline Configuration

The timeline can be configured from its popup menu. Use the right mouse button anywhere on the timeline to show the menu. If you show the popup menu by pointing directly at any part of the timeline, the popup menu will show that frame number, the source media there, and the operations will all be relative to that frame. For example, without changing frames you can set the in and out point or set a mark via the menu.

By default the timeline will show the \`\`source” frame number, the native number of the media. Alternately you can show the global frame number, global time code, or even the \`\`Footage” common in traditional animation (16 frames per foot).

![22_timelineMenuShot.jpg](../../images/rv-user-manual-22-rv-cx-timelineMenuShot-21.jpg)  

Figure 4.12: Timeline Configuration Popup Menu <a id="timeline-configuration-popup-menu"></a>

The Configuration menu has a number of options:

|  |  |
| --- | --- |
| Show Play Controls            | Hide or Show the playback control buttons on the right side of the timeline                                                                     |
| Draw Timeline Over Imagery    | This was the default behavior in previous versions of RV. The timeline is now drawn in the margin by default                                    |
| Position Timeline At Top      | Draw the timeline at the top of the view. The default is to draw it at the bottom of the view.                                                  |
| Show In/Out Frame Numbers     | When selected, the in and out points will be labeled using the current method for display the frame (global, source, or time code).             |
| Step Wraps At In/Out          | This controls how the arrow keys behave at the in and out point. When selected, the frame will wrap from in to out or vice versa.               |
| Show Source/Input at Frame    | When selected, the main media file name for the frame under the pointer (not the current frame) will be shown just above or below the timeline. |
| Show Play Direction Indicator | When selected, a small triangle next to the current frame indicates the direction playback will occur, when started.                            |

#### 4.5.3 Realtime versus Play All Frames

Control → Play All Frames determines whether RV should skip frames or not if it is unable to render fast enough during playback. Realtime mode (when play all frames is not selected) uses a realtime clock to determine which frame should be played. When in realtime mode, audio never skips, but the video can. When play all frames is active, RV will never skip frames, but will adjust the audio if the target fps cannot be reached.

When the timeline is visible, skipped frames will be indicated by a small red circle towards the right hand side of the display. The number in the circle is the number of frames skipped.

#### 4.5.4 In and Out Points

There are two frame ranges associated with each view in an RV Session: the start and end frames and the in and out frames (also known as in and out points). The in and out points are always within the range of the start and end frames. RV sets the start and end frames automatically based on the contents of the view. The in and out points are set to the start and end frames by default. However, you can set these points using the \`\`[“ and \`\`]'' keys, or by right-clicking on the timeline. You can also set an in/out range by shift-dragging with the left-button in the Timeline or the Timeline Magnifier. The in/out range displayed in the timeline can also be changed with the mouse, either by dragging the whole range (click down in the middle of the range), or by dragging one of the endpoints (click down on the endpoint).To reset the in and out points, use the \`\`\\'' key.

If frames have been marked, RV can automatically set the in and out points for you based on them (use the ctrl-right/left-arrow keys, or command-right/left-arrow on Mac).

#### 4.5.5 Marks

A mark in RV is nothing more than a frame number which can be stored in an RV file for later use. To toggle a frame as being marked, use Mark → Mark Frame (or use the \`\`m'' hotkey). The timeline will show marks if any are present.

While not very exciting in and of themselves, marks can be used to build more complex actions in RV. For example, RV has functions to set the in and out points based on marks. By marking shot boundaries in a movie file, you can quickly loop individual shots without selecting the in and out points for each shot. By selecting Mark → Next Range From Marks and Mark → Previous Range From Marks or using the associated hot keys \`\`control+right arrow” or \`\`control+left arrow” the in and out points will shift from one mark region to the next.

Marking and associated hot keys for navigating marked regions quickly becomes indispensable for many users. These features make it very easy to navigate around a movie or sequence and loop over part of the timeline. Producers and coordinators who often work with movie files of complete sequences (for bidding or for client reviews) find it useful to mark up movie at the shot boundaries to make it easy to step through and review each shot.

![23_timelineMarksShot.png](../../images/rv-user-manual-23-rv-cx-timelineMarksShot-22.png)  

Figure 4.13: Timline with Marks <a id="timline-with-marks"></a>

#### 4.5.6 Timeline Magnifier

The Timeline Magnifier tool (available from the **Tools** menu, default hotkey F3) brings up a special timeline that is \`\`zoomed'' to the region bounded by the In/Out Points. In addition to showing only the in/out region, the timeline magnifier differs from the standard timeline in that it shows frame ticks and numbers on every frame, if possible. If there is not enough room for frames/ticks on every frame, the magnifier will fall back to frames/ticks every 5 or 10 frames. The frame numbers of the in/out points are displayed at either end of the magnified timeline.

##### Audio Waveform Display

The timeline magnifier can display the audio waveform of any loaded audio. Note that this is the normalized sum of all audio channels loaded for the given frame range. To preserve interactive speed, the audio data is not rendered into the timeline until that section of the frame range is played. You can turn on **Scrubbing** , in the **Audio** menu, to force the entire frame range to be loaded immediately. Also, if **Scrubbing** is on, audio will play during scrubbing, and during single frame stepping. See Section [4.6](#46-audio) for further details on Audio in RV.

##### In/Out Range Manipulation

Note that on each end of the timeline magnifier, there are two triangular \`\`arrow” buttons. These are the in/out nudge buttons, and clicking on them will move the in or out point by one frame in the indicated direction. The in/out range displayed in the timeline can also be changed with the mouse, either by dragging the whole range (click down in the middle of the range), or by dragging one of the endpoints (click down on the endpoint). All these manipulations can be performed during playback. You can also set an in/out range by shift-dragging with the left-button in timeline magnifier.

##### Configuration

All the hotkeys mentioned in Table [4.5](#useful-timeline-hotkeys) are also relevant to the timeline magnifier. The timeline magnifier configuration menu is also a subset of the regular timeline menu (see Figure [4.12](#timeline-configuration-popup-menu) ), with additional items for setting the height of the audio waveform display.

![24_magnifierMenuShot.jpg](../../images/rv-user-manual-24-rv-cx-magnifierMenuShot-23.jpg)  

Figure 4.14:

Timeline Magnifier Configuration Popup Menu

### 4.6 Audio


When playing back audio with an image sequence or movie file, RV can be in one of two modes: video locked to audio or audio locked to video.

When a movie with audio plays back at its native speed, the video is locked to the audio stream. This ensures that the audio and video are in sync.

If you change the frame rate of the video, the opposite will occur: the audio will be locked to the video. When this happens, RV will synthesize new audio based on the existing audio in an attempt to either stretch or compress the playback in time. When pushed to the limits, the audio synthesis can create artifacts (e.g. when slowing down or speeding up by a factor of 2 or more).

RV can handle audio files with any sample rate and can re-sample on the fly to match the output sample rate required by the available audio hardware. The recommended formats are AIFF or WAV. Use of mp3 and audio-only AAC files is not supported.

Audio settings can be changed using the items on the Audio Menu. Volume, time Offset, and Balance can be edited per source or globally for the session. The RV Preferences Audio tab lets you choose the default audio device and set the initial volume (as well as some other technical options that are rarely changed).

For visualizing the audio waveform see Section [4.5.1](#451-timeline) .

#### 4.6.1 Audio Preferences

RV provides audio preferences in the Preferences dialog. The most important audio preference is the choice of the output device from those listed. In practice this will rarely change. Preferences also let you set the initial volume for RV. The option to hold audio open is for use on Linux installations where audio system support is poor (see the next section on Linux Audio.) The other preferences are there for fine tuning performance in extreme cases of marginal audio hardware or support - they will almost never change.

RV offers a cross-platform output module choice called “Platform Audio”. This is based on Qt audio. “Platform Audio” does support the use USB based audio peripherals for playback (e.g. Behringer UCA 202) on all platforms. These usb audio devices would typically appear as “USB Audio CODEC” (“front:CARD=CODEC,DEV=0” on Linux) in the “Output Device” pull down menu when “Platform Audio” is selected.

![25_audio_prefs_mac.png](../../images/rv-user-manual-25-rv-cx-audio-prefs-mac-24.png)  

Figure 4.15: Audio Preferences (macOS)

On the macOS and Windows there is only a single entry in this menu. On Linux, however, there may be many. (See Appendix [E](rv-user-manual-chapter-e.md#e-rv-audio-on-linux) for details about Linux Audio).

The Output Device, Output Channel Layout, Output Format, and Output Rate determine the sound quality and speakers (e.g. mono, stereo, 5.1 etc) to use. Typical output rates are 44100 or 48000 Hz and 32 bit float or 16 bit integer output format. This produced Audio CD or DAT quality audio.

Global Audio Offset is the means by which audio data can be time shifted backwards or forwards in time. The effect of this preference is observable in the audio waveform display. For example, setting the value to 0.5 seconds will shift the audio data by 0.5 seconds.

Device Latency allows you to correct for audio/video sync differences measured during playback. It is measured in milliseconds, and defaults to zero. The audio waveform rendered in RV is not affected by the value of this preference since it does not offset the audio data that is cached.

The Device Packet Size and Cache Packet Size can be changed, but not all output modules support arbitrary values. The default values are recommend. The Min/Max Buffer Size determines how much audio RV will cache ahead of the display frame. Ideally these numbers are low.

Keep Audio Device Open When Not Playing should usually be set to ON. There are very few circumstances in which it's a good idea to turn this off. When the value is OFF, RV may skip frames and audio when playback starts and can become unstable. On some linux distributions turning this OFF will result in no audio at all after the first play.

Hardware Audio and Video Synchronization determines which clock RV will use to sync video and audio. When on, RV will use the audio hardware clock if one is available otherwise it will use a CPU timer in software. In most cases this should be left ON. RV can usually detect when the audio clock is unstable or inaccurate and switch to the CPU timer automatically. However if playback with audio appears jerky (even when caching is on) it might be worth turning it off.

Scrubbing On By Default (Cache all Audio) determines if audio scrubbing is on when RV starts.

PreRoll Audio on Device Open generally improves the consistency of RV’s playback across different Linux machines and audio devices. It influences the overall AV sync lag, so expect to see different in AV sync readings when the feature is enabled versus turned off. In either case, the AV sync lag can be corrected via the Device Latency preference. Note that this feature is Linux only and available only for the Platform Audio module. It defaults to turned off.

#### 4.6.2 Audio on Open RV for Linux

Linux presents special challenges for multimedia applications and audio is perhaps the worst case. RV audio works well on Linux in many cases, but may be limited in others. RV supports special configuration options so that users can get the best audio functionality possible within the limitations of the vintage and flavor of Linux being used. See the Appendix [E](rv-user-manual-chapter-e.md#e-rv-audio-on-linux) for complete details.

#### 4.6.3 Multichannel Audio

In RV, the audio output module “Platform Audio” will support multichannel channel audio playback for devices that allow it. This would include six, eight or more channel layouts for surround sound speaker systems like 5.1 or 7.1. The list of available channel layouts for a chosen output device will be listed in the first pulldown menu for the “Output Format and Rate” setting.

The list of all possible channel layouts that RV supports is described in Appendix [J](rv-user-manual-chapter-j.md#j-supported-multichannel-audio-layouts) .

#### 4.6.4 Correcting for AV Sync Delay

The “Device Latency” preference is intended to be used to compensate for any positive or negative AV sync measured during playback. To correct for an AV sync lag, first measure the delay with an AV sync meter. Then input the number from the meter into the Device Latency preference.

**Please note:** The AV sync measurement can be influenced by the following audio preferences or playback settings: Hardware Audio Video Sync, PreRoll Audio on Device Open, and video/audio cache settings.

To generate a sync flash sequence for use in measuring the AV sync at a particular frame rate, the following RV command line can be used. This example generates a 500 frame sequence with an audio bleep/video flash at intervals of 1sec at 24 FPS:

```
rv syncflash,start=1,end=500,interval=1,fps=24.movieproc
```

### 4.7 Caching


RV has a three state cache: it's either off, caching the current in/out range, or being used as a look-ahead (also known as a ring) buffer.

![26_e_timeline_cache.png](../../images/rv-user-manual-26-rv-cx-e-timeline-cache-25.png)  

Figure 4.16: Timeline Showing Cache Progress

The region cache reads frames starting at the in point and attempts to fill the cache up to the out point. If there is not enough room in the cache, RV will stop caching. The region cache can be toggled on or off from the Tools menu or by using the shift-C hot key.

![27_ase_region_cache.png](../../images/rv-user-manual-27-rv-cx-ase-region-cache-26.png)  

Figure 4.17: Region Cache Operation With Lots of Memory

![28_region_cache_no_room.png](../../images/rv-user-manual-28-rv-cx-region-cache-no-room-27.png)  

Figure 4.18: Region Cache Operation During Caching With Low Memory

Look-ahead caching can be activated from the Tools menu or by using the meta-l hot key. The look-ahead cache attempts to smooth out playback by pre-caching frames right before they are played. If RV can read the files from disk at close to the frame rate, this is the best caching mode. If playback catches up to the look-ahead cache, playback will be paused until the cache is filled or for a length of time specified in the Caching preferences. At that point playback will resume.

![29_look_ahead_cache.png](../../images/rv-user-manual-29-rv-cx-look-ahead-cache-28.png)  

Figure 4.19: Look-Ahead Cache Operation

RV caches frames asynchronously (in the background). If you change frames while RV is caching it will attempt to load the requested frame as soon as possible.

If the timeline widget is visible, cached regions will appear as a dark green stripe along the length of the widget. The stripe darkens to blue as the cache fills. The progress of the caching can be monitored using the timeline. On machines with multiple processors (or cores) the caching is done in one or more completely separate threads.

**Note** that there is usually no advantage to setting the lookahead cache size to something large (if playback does not overtake the caching, a small lookahead cache is sufficient, and if it does, you probably want to use region caching anyway).

### 4.8 Color, LUTs, and CDLs


RV provides users with fine grained color management and can support various color management scenarios. See [7.1](rv-user-manual-chapter-seven.md#rv-pixel-pipeline) for detailed technical information about RV's color pipeline. Without adding any nodes the default graph in RV supports three LUTs and two CDLs per file, an overall display LUT, and has a number of useful color transforms built-in. You can load LUTs and CDLs using the File → Import menu (Display, Look, File, and Pre-Cache items), or you can drag and drop the files onto the RV window. Smart drop targets will allow you determine how the LUT or CDL will be applied. Note that there is no CDL slot for the display by default. See chapter [8](rv-user-manual-chapter-eight.md#8-using-luts-in-rv) for more information about using LUTs and [9](rv-user-manual-chapter-nine.md#9-using-cdls-in-rv) for using CDLs in RV.

RV's color transforms are separated into two menus. The Color menu contains transforms that will be applied to an individual source (whichever source is current in the timeline) and the View menu contains transforms that will be applied to all of the sources. This provides the opportunity to bring diverse sources (say Cineon Log files, QuickTime sRGB movies, and linear-light Exr's) all into a common working color space (typically linear) and then to apply a common output transform to get the pixels to the display. RV's built in hardware conversions can handle Cineon Log, sRGB, Viper Log and other useful transforms.

### 4.9 Stereo

RV supports playback of stereoscopic source material. RV has two methods for handling stereo source material: first any source may have multiple layers, and RV will treat the first two video layers of a source as left and right eyes for the purpose of stereo display. Left and right layers do not need to be the same resolution or format because RV always conforms media to fit the display window; Second, RV supports stereo QuickTime movies (taking the first two video tracks as left and right eyes) and multi-view EXR files. RVIO can author stereo QuickTime movies and multi-view EXR files as well, so a complete stereo publishing and review pipeline can be built with these tools. See the section on [12](rv-user-manual-chapter-twelve.md#12-stereo-viewing) for more information about how stereo is handled.

### 4.10 Key and Mouse Bindings

RV has many built-in shortcuts. You can learn about RV’s hotkeys via RV’s Help menu.

![Help menu](../../images/rv-hotkeys-help-menu-01.png)

From the Utilities section of the Help menu, select “Describe…” or “Describe Key Binding…” to see an explanation within RV of what certain hotkeys do.

![Describe options in RV](../../images/rv-hotkeys-describe-options-rv-02.png)

![Describe options](../../images/rv-hotkeys-describe-options-03.png)

Menu items with hotkeys also display the hotkey on the right side of the menu item.

![Hotkeys](../../images/rv-hotkeys-hotkeys-04.png)

If you’d like to see a list of all of RV’s current key bindings, select “Show Current Bindings” from the Help menu. Below is also a list of RV’s hotkeys (note that capital and lowercase letters are different hotkeys):

| Hotkey | Action |
| --- | --- |
| F1 (Fn + F1 on Mac) | Toggle Menu Bar Visibility |
| F2 (Fn + F2 on Mac) | Toggle Heads-Up Timeline |
| F3 (Fn + F3 on Mac) | Toggle Timeline Magnifier |
| F4 (Fn + F4 on Mac) | Toggle Heads-Up Image Info |
| F5 (Fn + F5 on Mac) | Toggle Heads-Up Color Inspector |
| F6 (Fn + F6 on Mac) | Toggle Wipes |
| F7 (Fn + F7 on Mac) | Toggle Heads-Up Info Strip |
| F8 (Fn + F8 on Mac) | Toggle Heads-Up External Process Progress |
| ~ | Toggle Timeline |
| ` | Toggle Fullscreen Mode |
| ! | Set image scale to 1:1 on presentation device |
| * | Apply Random Luminance LUT |
| ( | Cycle Image Stack Backwards |
| ) | Cycle Image Stack Forwards |
| [ | Set In Point |
| ] | Set Out Point |
| \| | Set In/Out Range From Surrounding Marks |
| \ | Reset In/Out Points |
| , | Set Frame Increment to -1 (reverse) |
| . | Set Frame Increment to 1 (forward) |
| < | Go to Matching Frame of Previous Source |
| > | Go to Matching Frame of Next Source |
| ? | Show Help Options |
| Space | Toggle Play |
| 1 | Scale 1:1 |
| 2 | Scale 2:1 |
| 3 | Scale 3:1 |
| 4 | Scale 4:1 |
| 5 | Scale 5:1 |
| 6 | Scale 6:1 |
| 7 | Scale 7:1 |
| 8 | Scale 8:1 |
| A | Toggle Real-Time Playback |
| a | Show Alpha Channel |
| B | Edit Display Brightness |
| b | Show Blue Channel |
| C | Toggle Region Caching |
| c | Normal Color Channel Display |
| D | Toggle Display LUT |
| e | Edit Current Source Relative Exposure |
| F | Enter FPS Value From Keyboard |
| f | Frame Image in View |
| G | Set Frame Number Using Keyboard |
| g | Show Green Channel |
| h | Edit Current Source Hue |
| i | Toggle Heads-Up Image Info |
| k | Edit Current Source Contrast |
| L | Toggle Cineon Log to Linear Conversion |
| l | Show Image Luminance |
| M | Cycle Matte Opacity |
| m | Toggle Mark At Frame |
| n | Toggle Nearest Neighbor/Linear Filter |
| P | Toggle Ping/Pong Playback |
| p | Toggle Premult Display |
| q | Close Session |
| R | Force Reload of Current Source |
| r | Show Red Channel |
| S | Edit Current Source Saturation |
| T | Toggle Current Luminance LUT |
| t | Toggle Heads-Up Timeline |
| v | Enter Display Gamma |
| W | Fit Window to Pixels |
| w | Resize Window to Fit |
| X | Flop Image |
| Y | Flip Image |
| y | Edit Current Source Gamma |
| Alt + f (Option + f on Mac) | Set image scale fit image width on presentation device |
| Alt + l (Option + l on Mac) | Rotate Image 90deg Counter-Clockwise |
| Alt + n (Option + n on Mac) | Turn On Nudge Keys |
| Alt + r (Option + r on Mac) | Rotate Image 90deg Clockwise |
| Alt + s (Option + s on Mac) | Turn On Stereo Keys |
| Alt + left arrow (Option + left arrow on Mac) | Go to Previous Marked Frame |
| Alt + right arrow (Option + right arrow on Mac) | Go to Next Marked Frame |
| Keypad-enter (Fn + enter/return on Mac) | Set Frame Number Using Keyboard |
| Home (Fn + left arrow on Mac) | Go to Beginning of In/Out Range |
| End (Fn + right arrow on Mac) | Go to End of In/Out Region |
| Page-up (Fn + up arrow on Mac) | Set In/Out to Next Marked Range |
| Page-down (Fn + down arrow on Mac) | Set In/Out to Previous Marked Range |
| Ctrl + e (Cmd + e on Mac) | Export Quicktime Movie |
| Ctrl + f (Cmd + f on Mac) | Frame Image Width |
| Ctrl + i (Cmd + i on Mac) | Add Source |
| Ctrl + l (Cmd + l on Mac) | Toggle Look-Ahead Caching |
| Ctrl + m (Cmd + m on Mac) | Cycle Mattes |
| Ctrl + o (Cmd + o on Mac) | Open File |
| Ctrl + p (Cmd + p on Mac) | Toggle Presentation Mode |
| Ctrl + q (Cmd + q on Mac) | Close Session |
| Ctrl + s (Cmd + s on Mac) | Save Session |
| Ctrl + v (Cmd + v on Mac) | Edit Global Audio Volume |
| Ctrl + w (Cmd + w on Mac) | Close Session |
| Ctrl + left arrow (Cmd + left arrow on Mac) | Set In/Out to Previous Marked Range |
| Ctrl + right arrow (Cmd + right arrow on Mac) | Set In/Out to Next Marked Range |
| Ctrl + up arrow (Cmd + up arrow on Mac) | Expand In/Out to Neighboring Marked Ranges |
| Ctrl + down arrow (Cmd + down arrow on Mac) | Contract In/Out from Neighboring Marked Ranges |
| Left arrow | Move Back One Frame |
| Right arrow | Step Forward 1 Frame |
| Up arrow | Toggle Forward/Backward Playback |
| Down arrow | Toggle Play |
| Tab | Toggle Heads-Up Timeline |
| Shift + home (Shift + Fn + left arrow on Mac) | Reset All Color |
| Shift + left arrow | Go to Previous View |
| Shift + right arrow | Go to Next View |

#### Stereo Hotkeys

RV has a supplementary "stereo hotkeys" mode in which an additional set of stero-related hotkeys are active. You enter or leave the mode with Alt-s (Option-s on Mac).  While this mode is active, the following additional hotkeys are available:

| Hotkey | Action |
| --- | --- |
| a | Anaglyph Mode  |
| d | Checked Mode  |
| k | Scanline Mode  |
| s | Side-by-Side Mode  |
| p | Side-by-Side Stereo Mode  |
| m | Mirrored Side-by-Side Stereo Mode  |
| x | Stereo Mode Off (DEPRECATED -- use alt-s (or option-s on the mac))  |
| h | Hardware Stereo Mode  |
| , | Left Eye Only Stereo Mode  |
| . | Right Eye Only Stereo Mode  |
| < | Left Eye Only Stereo Mode  |
| > | Right Eye Only Stereo Mode  |
| S | Swap Eyes  |
| o | Edit Global Relative Stereo Offset|
| z | Horizontal Squeezed Stereo Mode  |
| v | Vertical Squeezed Stereo Mode  |
| r | Edit Global Right-Eye Stereo Offset |
| O | Turn OFF Stereo Display Mode (in Controller Window)  |
| / | Reset Stereo Offsets |
| c | Edit Source/Clip Stereo Offset  |
| R | Edit Source/Clip Right-Eye-Only Stereo Offset |

#### Mouse Bindings

Key and mouse bindings as well as menu bar menus are loaded at run time. You can override and change virtually any key or mouse binding from a file called ~/.rvrc.mu if you need to. The bindings (and whole interface) that comes with RV are located at $RV_HOME/plugins/Mu/rvui.mu. Functions in this file can be called from ~/.rvrc.mu or overridden.

To override bindings, copy the file $RV_HOME/scripts/rv/rvrc.mu to ~/.rvrc.mu.

| Alt     | Ctrl     | Shift     | 1     | 2     | 3     | Wheel     | Function               |
| ------- | -------- | --------- | ----- | ----- | ----- | --------- | ---------------------- |
|         |          |           | ↓     |       |       |           | Toggle Play            |
|         |          |           |       | ↓     |       |           | Toggle Play Direction  |
|         |          |           | ⇄     |       |       |           | Scrub Frames           |
|         |          |           |       |       |       | ⇄         | Scrub Frames           |
|         | •        |           |       |       |       | ⇄         | Scrub Frames 10x       |
| •       | •        |           |       |       |       | ⇄         | Scrub Frames 100x      |
|         | •        |           | ⇄     |       |       |           | Zoom                   |
|         |          |           |       | ↻     |       |           | Translate              |
| •       |          |           | ↻     |       |       |           | Translate              |
| •       | •        |           | ↻     |       |       |           | Translate              |
| •       |          |           |       | ↻     |       |           | Translate (Maya style) |
| •       |          |           | ⇄     | ⇄     |       |           | Zoom (Maya style)      |
|         |          | •         | ↓     |       |       |           | Inspect Pixel          |
|         |          | •         | ↻     |       |       |           | Average Pixels         |
|         |          |           |       |       | ↓     |           | Pop-up Menu            |

• held, ⇄ drag left and right, ↓ push without drag, ↻ drag any direction.

Mouse button 1 is normally the left mouse button and button 3 is normally the right button on two button mice. Button 2 is either the middle mouse button or activated by pushing the scroll wheel on mice that have them.

**If you can't annotate with the tablet and stylus:** If you use various inputs to control RV, such as Wacom tablets, then sometimes there is an incompatibility with the events those inputs generate and Qt. Try turning ON *Treat Stylus Events as Mouse Events* from RV Preferences General tab.

### 4.11 Preferences File


RV stores configuration information in a preferences file in the user home directory. Each platform has a different location and possibly a different format for the file.

| OS       | File Location                              | File Format     |
| -------- | ------------------------------------------ | --------------- |
| macOS | ~/Library/Preferences/com.tweaksoftware.RV | Property List   |
| Linux    | ~/.config/TweakSoftware/RV.conf            | Config File     |
| Windows  | %APPDATA%/TweakSoftware/RV.ini             | INI File        |

Table 4.8: Preference File Locations

### 4.12 Audio Settings

The Output Module in the Audio tab under RV Preferences lists audio interfaces that RV can choose between to handle audio, and is specific to operating systems. Each operating system has its own platform specific option, but fairly recently we created a Qt based cross platform setting named Platform Audio. We encourage you to use this option for Output Module.

The Output Format and Rate configuration is an important setting, and you should choose a reasonable format and rate for the audio contained in the sources you most commonly review. Most of the time this is 32-bit float and either 44.1 or 48 kHz.

Using “Keep Audio Device Open When Not Playing” will help reduce slow down and pops when looping playback or when starting and stopping frequently. However, in some Linux distributions this can conflict with other applications using the audio system. Using “Hardware Audio and Video Synchronization” may help keep your media synced during playback, but could introduce small delays for the two systems to line up when starting playback.

#### Audio Scrubbing

Under the Audio menu is an option to enable Scrubbing. This can be turned on by default from the Audio tab of RV’s preferences. For animators that are doing frame by frame stepping through clips, Scrubbing for lip-sync and other types of critical audio event timing can be useful.

#### Offset Playback Timing of Open RV audio

On the Audio tab of RV’s preferences, you can offset the playback timing of all RV audio in Global Audio Offset. This setting exists in case your system has some kind of audio exclusive latency so that you cannot configure both audio and video latencies from the Video tab.

#### Synchronizing Audio and Video

When using RV to play an image sequence paired with an audio file you may find that RV appears to play your imagery faster (or slower) than the accompanying audio. This happens because the way RV works is based on assuming each source has a native frame rate and that audio files do not have a native frame rate, because they have no frames. If RV is unable to determine the frame rate of a source and no indication was given at load, then RV will use the Default FPS. This is set in the Preferences dialog on the General tab.

The Default FPS setting is used by both RV and RVIO, and it normally is only used for sources that have no discernible native frame rate, such as in an image sequence (without FPS metadata) or audio-only sources.
