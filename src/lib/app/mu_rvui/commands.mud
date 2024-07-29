
documentation: {
// symbols in global scope
MetaEvalInfo "Record of meta evaluation"
MetaEvalInfo.node "The name of the node"
MetaEvalInfo.nodeType "The type name of the node"
MetaEvalInfo.frame "The local frame number used during evaluation"

PixelImageInfo "Returned by the imagesAtPixel() function."
PixelImageInfo.name "The source name of the image"
PixelImageInfo.x "The screen x coordinate [-1, 1]"
PixelImageInfo.y "The screen y coordinate [-1, 1]"
PixelImageInfo.px "Pixel x coordinate"
PixelImageInfo.py "Pixel y coordinate"
PixelImageInfo.inside "True if the point querried is inside the image on screen"
PixelImageInfo.edge "Distance to the nearest edge on the image"
PixelImageInfo.modelMatrix "The 4x4 model matrix for the image"
PixelImageInfo.globalMatrix "The 4x4 global matrix (includes viewing transform) for the image"
PixelImageInfo.projectionMatrix "The 4x4 projection matrix used for rendering this image"
PixelImageInfo.textureMatrix "The 4x4 texture matrix used for rendering this image"
PixelImageInfo.node "The name of the node which created the image"
PixelImageInfo.tags "Array of string 2-tuples of name/value pairs of tags"
PixelImageInfo.index "The internal rendering index number of this image"
PixelImageInfo.pixelAspect "The pixel aspect ratio used to render this image"
PixelImageInfo.device "The name of the device the image is associated with"
PixelImageInfo.physicalDevice "The name of the physical device the image is associated with"
PixelImageInfo.serialNumber "The serial number of the render which produced the image"

RenderedImageInfo "Returned by the renderedImages() function"
RenderedImageInfo.name "The source name of the image"
RenderedImageInfo.index "The rendering index"
RenderedImageInfo.imageMin "The lower left corner in pixel space"
RenderedImageInfo.imageMax "The upper right corner in pixel space"
RenderedImageInfo.stencilMin "The lower left corner of the stencil rectangle in pixel space"
RenderedImageInfo.stencilMax "The upper right corner of the stencil rectangle in pixel space"
RenderedImageInfo.modelMatrix "The model matrix of the image"
RenderedImageInfo.globalMatrix "The global matrix (includes viewing transform) of the image"
RenderedImageInfo.projectionMatrix "The projection used to render the image"
RenderedImageInfo.textureMatrix "The texture matrix used to render the image"
RenderedImageInfo.width "The rendered image width in pixels"
RenderedImageInfo.height "The rendered image height in pixels"
RenderedImageInfo.bitDepth "The image bit depth"
RenderedImageInfo.floatingPoint "True if the image is a floating point format"
RenderedImageInfo.numChannels "The number of channels in the original image"
RenderedImageInfo.planar "True if the image was rendered using multiple image planes"
RenderedImageInfo.node "The source node that generated the image"
RenderedImageInfo.tags "An array of tag name/value pairs generated from the graph"
RenderedImageInfo.pixelAspect "The pixel aspect ratio of the image"
RenderedImageInfo.device "The name of the device the image is associated with"
RenderedImageInfo.physicalDevice "The name of the physical device the image is associated with"
RenderedImageInfo.serialNumber "The serial number of the render which produced the image"

SourceMediaInfo "Returned by sourceMediaInfo function"
SourceMediaInfo.width "The width of the image/sequence in pixels"
SourceMediaInfo.height "The height of the image/sequence in pixels"
SourceMediaInfo.uncropWidth "The uncrop width (aka display window width) of the image/sequence in pixels"
SourceMediaInfo.uncropHeight "The uncrop height (aka display window height) of the image/sequence in pixels"
SourceMediaInfo.uncropX "The uncrop x offset in pixels from the origin of image"
SourceMediaInfo.uncropY "The uncrop y offset in pixels from the origin of image"
SourceMediaInfo.bitsPerChannel "The number of bits per-channel, e.g. 8, 16, 32"
SourceMediaInfo.channels "Number of channels in the image"
SourceMediaInfo.isFloat "True if the image is a floating point format"
SourceMediaInfo.planes "Number of separate image planes"
SourceMediaInfo.startFrame "Start frame number of sequence or movie file"
SourceMediaInfo.endFrame "End frame number of sequence or movie file"
SourceMediaInfo.fps "FPS of sequence or movie file if known or 0"
SourceMediaInfo.audioChannels "Number of audio channels in the media"
SourceMediaInfo.audioRate "Audio sampling rate (e.g. 44100 or 48000)"

PropertyInfo """
A description of a property. 

*NOTE:* this type changed between rv-3 and rv-4. The +width+ field
was removed and replaced by +dimensions+ . The +dimensions+ field gives
the size of the property in each of its dimensions; there are up to
four dimensions. For a property with few than four dimensions, the
remaining dimensions will have size 0 to indicate that the dimension
is not being used (or degenerate depending on how you look at it).
"""

PropertyInfo.name "The name of the property"
PropertyInfo.type "The type name of the property"
PropertyInfo.dimensions "The sizes of each dimension of the property as an integer 4-tuple"
PropertyInfo.size "The number of elements in the property"
PropertyInfo.userDefined "Indicates whether the property was created by the user"
PropertyInfo.info "Descriptive information string"

NodeImageGeometry """
Description of a node's output image structure including size in
pixels, pixel aspect ratio, and a matrix which will transform the
image to the standard origin.
"""

NodeImageGeometry.width "Width in pixels"
NodeImageGeometry.height "Height in pixels"
NodeImageGeometry.pixelAspect "Pixel aspect ratio"
NodeImageGeometry.orientation "4x4 orientation matrix (really a 2x2 matrix)"

Event "Information about an event"

Event.pointer """
Returns the event space coordinates of a pointer event. This function
is only valid if called in response to a pointer event.
"""
Event.relativePointer """
Returns the event space coordinates of a pointer event relative to the
event table bounding box. This function is only valid if called in
response to a pointer event."""

Event.reference """
Return the event space coordinates of the pointer reference point
(down point).  This function is only valid if called in response to a
pointer event."""

Event.domain "Returns the size in width and height of the event domain"
Event.subDomain "Returns the size in width and height of the event table if there is one"
Event.buttons """
Returns the buttons states as a bit field. The button constants are:
Button1, Button2, Button3."""

Event.modifiers """
Returns the modifier key states as a bit field. Modifier constants
are: None, Shift, Control, Meta, Alt, Super, CapLock, NumLock,
ScrollLock."""

Event.key """
Returns the UCS-32 key value of a key up or down event. This function
is only valid if called in reponse to a key event."""

Event.name """
The name of the event. This is the name that the bind() function would take."""

Event.contents """
A string "payload" which is availble for drag-drop, application events, 
raw data, and render events. The information returned is specific to the event
type."""

Event.dataContents """
A byte array "payload" which is available from a raw data event."""

Event.returnContents """
Access to the "return value" for this event set by previous event handlers.  In order
to "daisy-chain" event handling for events with a return value (for example, the
"incoming-source-path" event), handlers should check the event's returnContents,
so that work done by previous handlers is not overridden."""

Event.sender """
The name of the object/application that sent the event. This could be the 
name of a network presence or a module."""

Event.contentType """
One of FileType, URLType, or TextType. Available from a drag-drop event."""

Event.contentMimeType """
A mime type string indicating how the dataContents() are encoded."""

Event.timeStamp """
A time in seconds when the event occured. This time 0 has no special meaning."""

Event.reject """
If the application finds an event binding and calls it and the event
is not rejected, then no further event functions will be called. If
the reject() is called on the event then futher bindings will be
considered.
"""

Event.setReturnContent """
In addition to content() and dataContent(), the event can also have a
return content. This is a string which may be set to indicate a
response to an event. Most events ignore this."""

Event.pressure "The stylus pressure."
Event.tangentialPressure "The stylus tangential pressure component."
Event.rotation "The stylus pen rotation."
Event.xTilt "The stylus tilt in the X direction."
Event.yTilt "The stylus tilt in the Y direction."


commands """
This module provides the most atomic API to RV. Other modules like `extra_commands` , `rvtypes` , 
`rvui` , etc are built using the `commands` module API.
"""

}

module: commands {
documentation: {


//
//  Low level commands
//
        
eval """
Evalutes a string as Mu source. The resulting value is converted to a
string and returned. Eval runs in the context of the application so
any modules loaded into the app will be visible to the code.
"""

activateMode """
Turn on the event table(s) for the given mode. After calling
activateMode() events will be filtered through the mode's event table.
"""

deactivateMode """
Turn off the event table(s) for the given mode.
"""

isModeActive """
Returns true if the given mode is active.
"""

activeModes """
Returns a list of active mode names.
"""

defineMinorMode """
Creates a (low level) minor mode and space for associated event
tables. The name is assigned to the mode. A mode at this level is an
object with the following attributes:

 * Name
 * Set of named event tables
 * Sort key and order number
 * Menu 

When a mode is active its menu is merged into the main menu of the
application and its event tables are used to handle events. The order
in which the mode's event tables are scanned is determined by the sort
key and the order number. The modes are sorted lexically using the
sort key; if two modes have the same sort key, then the order value is
used as the key the modes with common sort keys.

There are two types of modes: major and minor. There can be any number
of minor modes active at a time but only a single major mode, and the
active major mode has higher precedence than any minor mode when
scanning event tables (i.e. the major mode gets events first).

Each mode can have multiple named event tables which can be activated
independently as long as the mode itself is active. The activation
history of event tables is a stack which can be pushed and poped. Its
also possible to deactive them in any order if needed.

Each event table has a bounding box. The bounding box is only used
with pointer events or events that have a screen location. Only events
with a location that falls inside of the bounding box are considered.
"""

bindingDocumentation """
Returns the documentation associated with the given binding of
eventName in mode modeName and table tableName.
"""

bindings """
Returns a array of tuples of all currently active bound events and the
assocated documentation. This returns events from all currently active
event tables in the order in which they are scanned.
"""

bind """
Bind an event function to an event. If a the named event table does
not yet exist in the specified mode it is created. 

An event function must have the signature: (void; Event). The Event
object can be querried and state can be set on it in response to the
event. If the event function does not call the reject() function on
the event object, then the event is considered accepted and no further
matching event functions will be called. If, on the other hand,
reject() is called and another function is bound to the same event,
then it will also be called. This process continues until reject() is
not called (the event is accepted) or all bound event functions have
been called.

The description string should indicate succinctly (i.e. a sentence or
less) what the event function does.
"""

bindRegex """
Bind an event function to any event that matches the given posix
regular expression. Regular expression bindings have lower precedence
than normal bindings so they will never orderride a normal
binding in the same table.

See also: bind()
"""

unbind """
Removes the event binding from the given mode's event table.
"""

unbindRegex """
Removes the regular expression event binding from the given mode's
event table.
"""

setEventTableBBox """
Set the bounding box for the given mode's event table.
"""

defineModeMenu """
Define the menu structure associated with the mode. The menu is
automatically merged into the main menu when the mode is active. When
the mode is inactive the menu is not part of the main menu.

Setting the strict option to true will replace the menu instead of
merging with an existing menu of the same name.
"""

pushEventTable """
For all currently active modes, the named event table will be pushed
onto the event table stack. Since the new table is at the top of the
stack, it will be considered before any other event table. 

Event tables of the same name are merged together such that the mode
ordering determines binding precedence. Table ordering is determined
by the event table stack. Tables at the top of the stack can shadow
tables lower on the stack.

The event table stack is used to implement prefix keys. E.g. when a
prefix key is pressed it causes a specific event table to be pushed to
the top of stack thus overriding existing bindings. When the prefix
actions are complete the table is popped off the stack.
"""

popEventTable """
Causes the top of the event table stack to be popped off the
stack. The case in which a table name is specified, the named table is
removed from the event table stack without further changing the
ordering of the stack.
"""

activeEventTables """
Returns a list of the event table names current active.
"""

contractSequences """
Given an array of file names, the function will attempt to contract
them into image sequence notation. For example, the files:

----
foo.0001.tif
foo.0002.tif
foo.0003.tif
foo.0004.tif
----

would result in a single sequence named foo.#.tif. See the RV user's
manual for information about sequence notation.
"""

sequenceOfFile """
Given a single file name, the function will attempt to find a sequence
on the file system than contains it.
"""

//
//  RV specific commands
//


sessionName "Returns the name as seen by the user of the current session."
sessionNames "Returns the names of all sessions being managed by the current RV process."
setSessionName "Set the user visible session name."

setFrame """
Set the current frame. This will trigger a frame-changed event.
"""
markFrame """
Mark or unmark the given frame for the current view node. Marks are
stored independently on each view node.
"""

isMarked "Returns true if the frame is marked in the current view."
markedFrames "Returns an array of all frames marked in the current view."

scrubAudio """
Turn on/off audio scrubbing. The optional chunkDuration parameter
indicates the smallest length of contiguous audio that will be
played. The optional loopCount parameter indicates how many times a
chunk will be played if the user is holding the mouse down but not
moving it. 
"""

play """
Put RV into play mode. RV will be in one of realtime or
play-all-frames (realtime off) mode when it plays. You can set
realtime mode on or off using setRealtime().
"""

redraw """
Calling redraw will queue a redraw request for the session's main view.
"""

clearAllButFrame """
Clear the frame cache except for the given frame.
"""

reload """
Reload the frame cache. If no arguments are given, the entire frame
cache will be reloaded. If a global startFrame and endFrame are given,
only frames between them are reloaded.  For sequences, frames that didn't exist originally, but 
exist now and are part of the target range, will be loaded.
"""

loadChangedFrames """
For each in the given list of RVFileSource nodes, cached frames that have
changed on disk since they were last read will be reloaded (including the
current frame).  Only works with Sources that hold a sequence of image files.
"""

isPlaying """
Returns true if RV is in playback mode.
"""

stop """
Stop playback mode.
"""

frame "Return the current global frame number"

metaEvaluate """
This function scans the internal IP graph in the same manner that the
renderer does, but does not evaluate images in the process. Instead,
it records the names and evaluation input parameters for each node as
it is traversed in evaluation order. 

The frame passed in is usually the current frame, but can be any valid
global frame number.

You can optionally specify a root and leaf node between which the
meta-evaluation occcurs. By default it will use the root of the graph
and all leaves.

The function returns an array of MetaEvalInfo structures. 
"""

metaEvaluateClosestByType """
This function is similar to metaEvaluate() in that it scans the graph
in evaluation order. However, this function will stop scanning
branches in the IP graph when it encounters a node of type typeName.

The frame passed in is usually the current frame, but can be any valid
global frame number.

The optional root argument causes the function to start scanning at
that node instead of the true root of the graph. 

This behavior is similar to how the set*Property() functions decide
which node to use when given a type name instead of node name. E.g. 
`setIntProperty("#RVColor.lut.active", int[] {1})` . In that case, all 
"nearby" RVColor nodes are modified.
"""

closestNodesOfType """
This function will scan the graph in input order (determined by
scaning inputs in order) from the given root node or the current view
node if none is specified.

When a node matching the given type is found, and the depth is 0, the
branch will stop being searched for additional nodes and the next
branch will be searched.

For depth greater than 0, the function will continue searching
branches until N nodes of the type are found where N is the depth + 1.

Basically, this function is useful to find the nearest nodes to the
view node of a given type without worrying about evaluation order. So
in that sense it is similar to `metaEvaluateClosestByType()` . For
example this function is useful to find the nearest `RVPaint` nodes to
modify for annotation or to find the nearest transform nodes that
would be modifiable for a transform manipulator.
"""

mapPropertyToGlobalFrames """
This function performs an inverse mapping of frame numbers stored in a
property of a node to global frames in the current view. 

For example if you need to know the global frame corresponding to the local
frame 100 of media in a source node you can use this function. In this case an
int property on the source node needs to be created and set to the value 100.
In the simplist case, calling the function with the appropriate path to the
property will return an array containing a single value -- the global frame
corresponding to frame 100.   Note that in the more general case a single
source frame can correspond to any number of global frames.

The maxDepth argument can be used to limit the complexity of the
search -- this is useful if you know that more than one node in the
evaluation path has the given property. 

The optional root argument can be used to map to frames relative to
the given root instead of the true root of the graph. In that case the
returned frame numbers may not be "global" frames.
"""

frameStart "Returns the minimum frame of the frame range"
frameEnd "Returns the maximum frame of the frame range"

setRealtime "Turn on/off realtime playback mode"
isRealtime "Returns true if the session is in realtime playback mode"
skipped """
Returns the number of frames skipped during playback. This will be 0
if no frames were skipped.
"""

isCurrentFrameIncomplete "Returns true if one of rendered frames is incomplete (not all pixels are available)."

isCurrentFrameError "Returns true if an error occured trying to render one of the current frames"

data "Returns the object given to setData(). This is the rvtypes.State object."

inPoint "Returns the in-point. This value will be in the range of frameStart() and frameEnd()."
outPoint "Returns the out-point. This value will be in the range of frameStart() and frameEnd()."

setInPoint "Set the in-point. The value is clamped to the range [frameStart(), frameEnd()]."
setInPoint "Set the out-point. The value is clamped to the range [frameStart(), frameEnd()]."

setMargins """Set the rendering margins. This is a single float[4] vector encoding
the left, right, top and bottom margin sizes.  If any provided margin value is -1 the existing
margin value will not be changed.  The margins on only the "event device" will be changed
unless the allDevices arg is true.
"""

margins "Return the margin set via setMargins()."

setPlayMode """
Sets the looping behavior during playback. The accepted values are
PlayLoop, PlayOnce, and PlayPingPong.
"""

playMode "Returns the play mode set by setPlayMode()."

setFPS """
Set the playback frames/second. If this FPS does not match the FPS
of the imagery being viewed, RV will adjust any audio to maintain synchronization.
By default the FPS of a view matches what is being viewed.
"""

setFrameStart "Set the frame range start frame."
setFrameEnd "Set the frame range end frame."
realFPS "Returns the approximate real playback FPS as measured by the renderer."
fps "Returns the current playback FPS."
mbps "Returns the megabytes/second achieved by supported I/O methods."
resetMbps "Reset the running mbps tally."
setInc "Set the playback increment. This is typically -1 or 1 indicating forward or reverse playback."
setFiltering "Set the OpenGL filtering type. This is currently either GL_NEAREST or GL_LINEAR."
getFiltering "Return the filtering method set by setFiltering"
setCacheMode "Sets the frame caching mode. Accepted values are CacheOff, CacheGreedy, or CacheBuffer."
releaseAllUnusedImages "Forces flushing and deallocation of cached image data not currently wired to be used."
releaseAllCachedImages "Forces flushing and deallocation of all cached image data."
setAudioCacheMode "Set the audio cache mode. Accepted values are CacheOff, CacheGreedy, or CacheBuffer."
audioCacheMode "Returns the value set by setAudioCacheMode()."
cacheMode "Returns the value set by setCacheMode()."
isCaching "Returns true if images are being cached."
cacheInfo """
Returns a tuple of information about cache usage:

|===================================================
| int64 | capacity of the cache in bytes
| int64 | bytes used by cache
| int64 | look ahead bytes used
| float | look ahead seconds
| float | look ahead wait time in seconds
| float | audio cache time in seconds
| int[] | array of start-end frames of cached ranges
|===================================================
"""

isBuffering "Returns true if the renderer is paused to allow cache buffering to occur."
inc "Returns the value set by setInc()."
cacheSize "Returns the available memory."

sources """
Returns an array of source media. The return tuples contain:

|==================================
| string | file name
| int	 | start frame
| int	 | end frame
| int	 | frame increment
| float	 | fps
| bool	 | true if media has audio
| bool	 | true if media has video
|==================================
"""

fullScreenMode "Sets the interface to fullscreen or windowed mode."

isFullScreen "Returns true if the interface is in fullscreen mode."

eventToImageSpace """
Given a point in event space (whatever the Event object returns for
the pointer value) and the name of a source node/path, returns the
coordinate in local image coordinates.
"""

eventToCameraSpace """
Given a point in event space (whatever the Event object returns for
the pointer value) and the name of a source node/path, returns the
coordinate in camera space [0, aspect] x [0, 1].
"""

imagesAtPixel """
Given a point in event space (whatever the Event object returns for
the pointer value) returns an array of PixelImageInfo objects
describing the resulting rendered images at that pixel.

The PixelImageInfo objects will be in order of front to back.
"""


sourcesAtFrame """
Returns an array of the names of source nodes (RVFileSource or
RVImageSource) which would be evaluated at the given frame.
The array is guaranteed to contain only unique names (no duplicates).
"""

renderedImages """
Returns an array of RenderedImageInfo objects one for each image
rendered rendered to the view.
"""

imageGeometry """
Returns an array of image corners for the given source name. These are
the coordinates of the final rendered image in the view.
"""

imageGeometryByIndex """
Returns an array of image corners. These are the coordinates of the
final rendered image in the view. The index argument is the same one
found in the RenderedImageInfo and PixelImageInfo structures.
"""

imageGeometryByTag """
Returns an array of image corners. These are the coordinates of the
final rendered image in the view. The tag name and value are used
to locate the image.
"""

sourceMedia """
Return the media in the given source and a list of views and layers if
the media has any. For example a single EXR file in a source may have
multiple layers (e.g. diffuse, specular, etc) as well as multiple
views (e.g. left, right eyes). 
"""

sourcePixelValue """
Returns the value of a pixel in the original source image. This value
is converted to Rec. 709 RGBA form. The coordinates passed in are
relative to the original image pixel space.
"""

sourceAttributes """
Returns an array of image attribute name/value pairs at the current
frame. The sourceName can be the node name or the source path as
returned by PixelImageInfo, etc. The optional media argument can be
used to constrint the attributes to that media only.

Returns an empty array if no attributes are found.
"""

sourceDataAttributes """
Returns an array of image attribute name/value pairs at the current
frame for data blob attributes. The sourceName can be the node name or the source path as
returned by PixelImageInfo, etc. The optional media argument can be
used to constrint the attributes to that media only.

The name/value pair value is a byte[]. Unlike the sourceAttributes() function, 
only attributes of that type are returned. One use case is
to obtain the data blob for an embedded ICC profile.

Returns an empty array if no attributes are found.
"""

sourceMediaInfo """
Returns a SourceMediaInfo structure for the given source and optional
media. The SourceMediaInfo supplies geometric and timing information
about the image and sequence.
"""

sourceDisplayChannelNames """
Returns the names of channels in a source which are mapped to the display RGBA channels.
"""

getCurrentImageSize "Deprecated. Use sourceMediaInfo() instead."
getCurrentPixelValue "Deprecated. Use sourcePixelValue() instead."
getCurrentAttributes "Deprecated. Use sourceAttributes() instead."
getCurrentImageChannelNames "Deprecated. Use sourceMediaInfo() instead."
getCurrentNodesOfType "Deprecated. Use metaEvaluate() instead."

nodesOfType """
Returns all nodes of the given type. In addition to exact node type
names, you can also use #RVSource which will match RVFileSource or
RVImageSource.
"""

addSource """
Add a new source group to the session. This results in the creation of
an RVFileSource node and additional RVSourceGroup nodes like color, etc. 
A "incoming-source-path" event is generated when this is called.
A "new-source" event will also be generated after "incoming-source-path".

If an array of filenames are provided, they are added as layers in a single
source.  For example left and right eyes, or audio and video files.

An optional tag can be provided which is passed to the generated
internal events. The tag can indicate the context in which the
addSource() call is occuring (e.g. drag, drop, etc).
"""

addSourceBegin """
Optional call providing a fast add source mechanism when adding multiple 
sources which postpones connecting the added sources to the default 
views' inputs until after the corresponding addSourceEnd() is called.
The way to enable this optimization is to call addSourceBegin() first, 
followed by a bunch of addSource() calls, and then end with addSourceEnd().
"""

addSourceEnd """
Optional call providing a fast add source mechanism when adding multiple 
sources which postpones connecting the added sources to the default 
views' inputs until after the corresponding addSourceEnd() is called.
The way to enable this optimization is to call addSourceBegin() first, 
followed by a bunch of addSource() calls, and then end with addSourceEnd().
"""

addSources """
Add a new source group to the session (see addSource). This function adds
the requested sources asynchronously.  In addition to the "incoming-source-path" and 
"new-source" events generated for each source.  A "before-progressive-loading" and 
"after-progressive-loading" event pair will be generated at the appropriate times.

An optional tag can be provided which is passed to the generated
internal events. The tag can indicate the context in which the
addSource() call is occurring (e.g. drag, drop, etc).

The optional processOpts argument can be set to true if there are 'option' states
like -play, that should be processed after the loading is complete.

Note that sources can be single movies/sequences or you can use the "[]"
notation from the command line to specify multiple files for the source, like
stereo layers, or additional audio files.  Per-source command-line flags can
also be used here, but the flags should be marked by a "+" rather than a "-".
Also note that each argument is a separate element of the input string array.  
For example a single stereo source might look like string[] 
{"[", "left.mov", "right.mov", "+rs", "1001", "]" }
"""

addSourceVerbose """
Similar to addSource(), but returns the name of the source node created.
"""

addSourcesVerbose """
Similar to addSources(), but returns the names of the source nodes created.
Note that unlike addSources(), addSource(), and addSourceVerbose() which all
take a string[] as input parameter, addSourcesVerbose() takes a string[][].

Examples using addSourcesVerbose():
rv.commands.addSourcesVerbose([["clip1"], ["clip2"]])
rv.commands.addSourcesVerbose([["clip1_left", "clip1_right"], 
                               ["clip2_left", "clip2_right"]])

Examples using addSources():
rv.commands.addSources(["clip1", "clip2"])
rv.commands.addSources(["[", "clip1_left", "clip1_right", "]", 
                        "[", "clip2_left", "clip2_right", "]"])
"""

addSourceMediaRep """
Add a media representation to an existing source specified by sourceNode with an optional tag.
It returns the name of the source node created.
The source node for which to add the media representation can be set to "last" in which case it 
refers to the last created source.
For efficiency purpose, the media associated to this new media representation will NOT get
loaded until this source media representation becomes active via the setActiveSourceMediaRep()
command.
Note that an error will be thrown if the source media representation to be added already exists. 
See also: setActiveSourceMediaRep(), sourceMediaRep(), sourceMediaReps().

Examples:

rv.commands.addSourceMediaRep("last", "Frames", ["image_sequence.195-215#.jpg"])

rv.commands.addSourceMediaRep("sourceGroup000000_source", "Movie", ["left.mov", "right.mov"])

rv.commands.addSourceMediaRep("last", "Movie", ["movie.mov", "+rs", "194", "+pa", "0.0", "+in", "195", "+out", "215"])

It is also possible to add a source media representation in RV without calling
the addSourceMediaRep() command by specifying the "mediaRepName" and "mediaRepSource" 
options to the addSources(), addSourceVerbose(), or addSourcesVerbose() commands.

Example:

rv.commands.addSourcesVerbose(["image_sequence.195-215#.jpg", "+mediaRepName", "Frames", "+mediaRepSource", "last"])

"""

setActiveSourceMediaRep() """
Set the active input of the Switch node specified or the ones associated with 
the specified source node to the given media representation specified by name.
When sourceNode is an empty string "", then the media in the first Switch node 
found at the current frame is swapped with the media representation specified 
by name.
When sourceNode is "all", then the media in all the Switch nodes created with 
the rvc.addSourceMediaRep() command is swapped with the media representation 
specified by name.

Examples:

Set the active media representation for the current source to 'Streaming':
rv.commands.setActiveSourceMediaRep("", "Streaming")

Set the active media representation for all the sources to 'Streaming'
rv.commands.setActiveSourceMediaRep("all", "Streaming")

"""

sourceMediaRep() """

Returns the name of the media representation currently selected by the Switch 
Group corresponding to the given RVFileSource node.
When sourceNode is an empty string "", then the name of the currently selected 
media representation corresponding to the first Switch node found at the current 
frame is returned.

"""

sourceMediaReps() """

Returns the names of the media representations available for the Switch Group 
corresponding to the given RVFileSource node.
When sourceNode is an empty string "", then the first Switch node found at the 
current frame is used to determine the available media representations.

"""

sourceMediaRepsAndNodes() """

Returns an array of (mediaRepName, mediaRepSourceNode) pairs available for the
switch node specified as parameter containing the media representations.
Each pair consists of firstly the name of the media representation, and secondly
of the source node of that media representation.
Note that for convenience, a source node can be specified instead of the switch 
node, in which case the associated switch node will be inferred.

Returns an empty array if no media representations are found.

"""

sourceMediaRepSwitchNode() """

Returns the name of the switch node associated with the given source node if any.
Otherwise returns an empty string.

"""

sourceMediaRepSourceNode() """

Returns the name of the source media rep's switch node's first input associated 
with the source if any.
Otherwise returns an empty string.

"""

setProgressiveSourceLoading """
Turn on/off progressive source loading

Default=false unless 
overridden on the command line with -progressiveSourceLoading 1 
or 
via the RV_PROGRESSIVE_SOURCE_LOADING environment variable

progressiveSourceLoading() can be used to inquire the current progressive source loading state.

Note: this command affects the behaviour of the following scripting commands:
addSource(), addSources(), addSourceVerbose(), addSourcesVerbose().

When progressive source loading is disabled (default), the sources are loaded synchronously.
You can expect this sequence of events:
1) before-progressive-loading
2) source-group-complete media 1
3) source-group-complete media 2
4) after-progressive-loading

When progressive source loading is enabled, the sources are loaded asynchronously by following these steps:
1. RV creates a movie placeholder in the graph called a movieProxie which has a default duration of 20 frames.
2. It dispatches the actual loading of the media as a work item to a pool of worker threads.
3. Once the media completes the loading operation, the movieProxie placeholder is replaced with the actual movie.

You can expect this sequence of events:
1) before-progressive-loading
2) before-progressive-proxy-loading
3) source-group-proxy-complete media_1
4) source-group-proxy-complete media_2
5) after-progressive-proxy-loading
6) source-group-complete media 1
7) source-group-complete media 2
8) after-progressive-loading

Note that enabling progressive source loading changes the way you need to approach your scripts.

To illustrate this point, let's consider the following example:
rv.commands.addSource('/my/clip/1')
rv.commands.setFPS(60.00)

When progressive source loading is disabled (default), the script will behave as expected : the source will be loaded and the frame rate will be set to 60 fps.

When progressive source loading is enabled however, the source will be loaded asynchronously, the frame rate will be set momentarily to 60 fps, and then it will be set to the source native frame rate once the source is loaded.
When using this progressive source loading to increase the responsiveness of the app, you’ll now need to call the FPS set on the “after-progressive-loading” event to ensure loading is complete before setting a property. This will ensure that you can benefit from the feature while also ensuring your code runs correctly.
"""

progressiveSourceLoading """
Returns the current progressive source loading state
"""

newImageSource """
A new source group is created with an RVImageSource node holding the
image pixels. The arguments are used to construct the RVImageSource.
Once the image source is created you can call newImageSourcePixels()
to add blocks of pixels to it.
"""

newImageSourcePixels """
Allocate pixels for frame (and possible layer and view) in an existing
RVImageSource node. This should be called before
insertCreatePixelBlock().
"""

insertCreatePixelBlock """
Add a block of pixels to an existing RVImageSource node. This is
only used to pass along data from a "pixel-block" event (usually from
a network connection to an external process). 
"""

addToSource (void; string, string) """
Add media to the current source with an optional tag. See addSource()
for information about the tag.
"""

addToSource (void;string,string,string) """
Add media to the specified source with an optional tag. See
addSource() for information about the tag.
"""

setSourceMedia """
Replace all media in the given RVFileSource node with new media with optional tag.
"""

relocateSource (void;string,string) """
Replace media in the current source with new media.
"""

relocateSource (void;string,string,string) """
Replace media in the given source with new media.
"""

sessionFileName "Returns the name of the .rv file associated with the session."

setSessionFileName (void;string) """
Set the internal .rv file name for the session.
This does not save a session on disk. This is useful for defaulting the
File>Save to a location for a file that you wish to overwrite by default.
"""

commandLineFlag """
The RV command line has an option `-flags` . Arguments after `-flags` can
have the form `foo` or `foo=bar` . `commandLineFlag()` will find the value 
of one of these name value pairs. If the argument does not have a value, 
then true will be returned.

For example:

----
shell> rv -flags A B=foo
----

`commandLineFlag("A")` would return `true`

`commandLineFlag("B")` would return `foo`
"""

undoPathSwapVars """document me"""
redoPathSwapVars """document me"""

saveSession """
Save a .rv file for the session as fileName. If asACopy is true then
fileName will not become the session's file name. If compressed is
true the file will be a compressed binary .rv file otherwise it will
be human readable UTF-8 encoded text file.  If sparse is true the
session file will not contain most properties that still have their
default values.
"""

newSession """
Create a new session from the given files. The files may be media files or a single session file.
"""

clearSession """
Clear all sources from the current session. This will result in an empty session.
"""

setSessionType "Deprecated. Use setViewNode() instead."
getSessionType "Deprecated. Use viewNode() instead."

readLUT """
TBD
"""

getByteProperty """
Returns the value of a byte type property in the graph. Optional start and num 
parameters will cause the function to return a subset of the property if it has 
more than one element.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

getFloatProperty """
Returns the value of a float type property in the graph. Optional start and num 
parameters will cause the function to return a subset of the property if it has 
more than one element.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

getIntProperty """
Returns the value of a int type property in the graph. Optional start and num 
parameters will cause the function to return a subset of the property if it has 
more than one element.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

getStringProperty """
Returns the value of a string type property in the graph. Optional start and num 
parameters will cause the function to return a subset of the property if it has 
more than one element.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

getHalfProperty """
Returns the value of a half type property in the graph. Optional start and num 
parameters will cause the function to return a subset of the property if it has 
more than one element.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

setByteProperty """
Set the value of a property in the graph. The number of elements in the 
property and the number of elements in the value must match unless the optional
allowResize argument is true.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""
setFloatProperty """
Set the value of a property in the graph. The number of elements in the 
property and the number of elements in the value must match unless the optional
allowResize argument is true.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

setIntProperty """
Set the value of a property in the graph. The number of elements in the 
property and the number of elements in the value must match unless the optional
allowResize argument is true.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

setStringProperty """
Set the value of a property in the graph. The number of elements in the 
property and the number of elements in the value must match unless the optional
allowResize argument is true.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

setHalfProperty """
Set the value of a property in the graph. The number of elements in the 
property and the number of elements in the value must match unless the optional
allowResize argument is true.

If the property name starts with a '#' then it is considered a node type and
the first node in the evaluation path of that type will be selected. There is 
also a special value #View which will substitute in the name of the viewNode()
as a convenience.
"""

insertByteProperty """
Modify the value of a property in the graph. The value passed in replaces 
and/or extends parts of the current property value. The optional beforeIndex 
parameter specifies where the insertion will occur. By default the insertion is 
at the end of the property value. In other words, the default behavior is to 
append values onto the end of the property.

See also: setByteProperty
"""

insertFloatProperty """
Modify the value of a property in the graph. The value passed in replaces 
and/or extends parts of the current property value. The optional beforeIndex 
parameter specifies where the insertion will occur. By default the insertion is 
at the end of the property value. In other words, the default behavior is to 
append values onto the end of the property.

See also: setFloatProperty
"""

insertHalfProperty """
Modify the value of a property in the graph. The value passed in replaces 
and/or extends parts of the current property value. The optional beforeIndex 
parameter specifies where the insertion will occur. By default the insertion is 
at the end of the property value. In other words, the default behavior is to 
append values onto the end of the property.

See also: setHalfProperty
"""

insertStringProperty """
Modify the value of a property in the graph. The value passed in replaces 
and/or extends parts of the current property value. The optional beforeIndex 
parameter specifies where the insertion will occur. By default the insertion is 
at the end of the property value. In other words, the default behavior is to 
append values onto the end of the property.

See also: setStringProperty
"""

insertIntProperty """
Modify the value of a property in the graph. The value passed in replaces 
and/or extends parts of the current property value. The optional beforeIndex 
parameter specifies where the insertion will occur. By default the insertion is 
at the end of the property value. In other words, the default behavior is to 
append values onto the end of the property.

See also: setIntProperty
"""

newProperty """
Create a new property in the given node with the given name. 
The type is one of: IntType, FloatType, ShortType, StringType, HalfType, ByteType.
The width can be greater than 1 if a vector of type is intended. For example, a property 
that holds 3D points would be a FloatType with width 3.

See also commands.newNDProperty for a more general property constructor.
"""

newNDProperty """
Create a new multi-dimensional property in the given node with the
given name.  The type is one of: IntType, FloatType, ShortType,
StringType, HalfType, ByteType.  

The dimensions are specified with an integer 4 tuple. For example, a
property that holds a 32^3 3D LUT with color at each voxel might be
FloatType with dimensions (3,32,32,32). Degenerate dimensions should
have a value of 0. So a two dimensional property of 10x20 floats would
have dimensions (10,20,0,0). Dimensions of size 0 can only be followed
by dimensions of size 0.
"""

deleteProperty "Delete a property created with newProperty()."
nodes "Returns an array of strings containing the name of every node in the graph."
nodeType "Returns the type name of the given node."
deleteNode "Delete the node created by newNode()."
properties "Return an array of property names in the given node."
propertyInfo "Returns a PropertyInfo structure describing the given property."

propertyExists "Returns true if the specified property exists"
viewSize "The size in pixels of the viewing window"
startTimer "Start user timer."
elapsedTime "Elapsed time since user timer start in seconds."
theTime "The time in seconds since the application started."
stopTimer "Stop the user timer."
isTimerRunning "Returns true if startTimer() has been called more recently than stopTimer()."
updateLUT """
Deprecated. It is no longer necessary to use this function. It is now sufficient to use
commands.readLUT alone or set the LUT file path for a given LUT node.
"""
bgMethod "Return the background draw method as a string."
setBGMethod """
Set the background drawing method to one of: black, checker, grey18, grey50, or crosshatch.
"""
setRendererType "Unused."
getRendererType "Unused."
exportCurrentFrame """
Take a snapshot of the current view window and save it as an image file.
The file name extension is used to determine the format.
"""

exportCurrentSourceFrame """
Save the current source image as an image file.
The file name extension is used to determine the format.
"""

setHardwareStereoMode "Turn on/off quad-buffered OpenGL stereo."
fileKind """
Returns one of: ImageFileKind, MovieFileKind, LUTFileKind, DirectoryFileKind,
RVFileKind, EDLFileKind, or UnknownFileKind to indicate the kind of file
filename represents."""

optionsPlay """
Returns 1 if -play was on the command line or the preferences are set 
to play automatically."""

optionsPlayReset """
Reset -play option so that the subsequence optionsPlay calls will return 0."""

optionsNoPackages """
Returns 1 if -nopackages was on the command line."""

optionsProgressiveLoading "Deprecated."

audioTextureID """
Returns the OpenGL texture identifier for the audio texture
if it was created for display. 

The audio texture will be created if an RVSoundTrack node has its 
visual.width, visual.height, visual.frameStart and visual.frameEnd
properties set to reasonable values. If these are all set to 0, the
audio wave form texture is not created and the audioTextureID will
be invalid.
"""

sendInternalEvent """
Send an application generated event to the user interface. The event
is sent immediately. The event can optionally be sent with a string
that is retrievable via the Event.contents() function and a senderName
retrievable with the Event.sender() function.
"""

previousViewNode "The name of the last view node in the view history."
nextViewNode "The name of the next view node in the view history."
setViewNode "Set nodeName to be the current view node. The node must be a top level node."
viewNodes "Returns an array of the nodes which can be viewed."
viewNode "Returns the currently viewed node."
flushCacheNodeOutput """
Causes nodes with the given property to notify the cache that any cached
images associated with them should be flushed."""

nodeImageGeometry """
Return a NodeImageGeometry structure for the given node.
This structure describes the geometry and timing of the node's output.
"""

newNode """
Create a new node of type typeName with name nodeName. If nodeName is 
the name of an existing node it will be modified to make it unique.
Returns the name of the new node.

The event "new-node" will be sent immediately.
"""

nodeConnections """
Returns a tuple of two string arrays. The first string array contains
the names of the nodes connected to the inputs and the second array 
the names of nodes connected to the outputs."""

nodesInGroup """
Returns the names of nodes in the group node nodeName. These nodes
are by definition not top level nodes."""

nodeGroup """
Return the group node name that nodeName is a member of or "" if
nodeName is a top level node (not in a group). 
"""

nodeExists "Returns true if nodeName exists in the graph"

setNodeInputs """
Set the inputs for nodeName to the inputNodes. 
If the inputs would cause a cycle in the graph an exception will be thrown.

The event "graph-node-inputs-changed" will be sent immediately.
"""

testNodeInputs """
Return true if setting the inputs of nodeName to inputsNodes would cause
an exception to be thrown.
"""

contentAspect """
Returns a float value for the aspect ratio of the currently viewed source. If
the view contains multiple sources then the first rendered image is used.
"""

loadTotal "Total number of sources to be created during progressive loading."
loadCount "Number of sources loaded since progressive loading started."

//
//  UI commands
//

resizeFit "Cause the window geometry to be resized to the contents if possible."
setViewSize "Set the view area size to an exact width and height."
popupMenu "Display a popup menu at the pointer location in event."
setWindowTitle "Set the title of the window to title. On linux the window manager can override this."
center "Center the window on screen."
close "Close the session window and exit the session."
toggleMenuBar "Toggle menu bar visibility. Has no effect on the Mac."
isMenuBarVisible "Return true if the menu bar is visible."

openMediaFileDialog """
Launch the media file dialog to choose files. 

If associated is true on the Mac, the file dialog will be a "sheet" on 
other platforms it has no effect. 

The parameter selectType determines the selection behavior. It can be
one of: OneExistingFile, ManyExistingFiles, ManyExistingFilesAndDirectories,
OneFileName, and OneDirectory.

The optional filter parameter can be a regular expression to restrict the
files shown.

The optional defaultPath parameter can be used to set the default directory
shown in the dialog to a location other than the current working directory.

The optional label parameter can be set to a descriptive string which is
displayed in the dialog.

The function will return after the user has selected one or more files or 
an exception will be thrown if the user hit the cancel button. 

An array of the selected file names is returned.
"""

openFileDialog """
Launch a generic file dialog to choose files.

If associated is true on the Mac, the file dialog will be a "sheet" on 
other platforms it has no effect. 

The optional multiple parameter indicates whether multiple files or directories may
be selected.

The optional directory parameter indicates whether directories or files selection
should be allowed. If directory is true then files cannot be selected.

The optional filter parameter is a string of of the form:

----
ext1|desc1|ext2|desc2|ext3|desc3|...
----

The string contains pairs of extensions and descriptions. The pairs
are separated by pipe characters and the extension and descriptions
are also separated by pipes.

Alternate, the second form of the function can take a list of string pairs of `(extension, description)`

The file browser will add two additional choices for the user: "all relevant files" and "any file".
The first choice will allow any file that matches an extension in the list. The second will allow
any file. 

The optional defaultPath parameter can be used to set the default directory
shown in the dialog to a location other than the current working directory.
"""

saveFileDialog """
Launch a generic file or directory save dialog to select 
a file or directory name for saving.

If associated is true on the Mac, the file dialog will be a "sheet" on 
other platforms it has no effect. 

The optional filter parameter can be a regular expression to restrict the
files shown.

The optional defaultPath parameter can be used to set the default directory
shown in the dialog to a location other than the current working directory.

The optional directory parameter indicates whether directories or files selection
should be allowed. If directory is true then files cannot be selected.
"""

setCursor """
Set the view cursor to one of: CursorNone, CursorArrow, or CursorDefault.
Alternately, setCursor can accept one of the Qt cursor symbolic constants.
"""

stereoSupported "Returns true if quad-buffered stereo is supported by the hardware"

alertPanel """
Launch a dialog box to alert the user to something important.

If associated is true on the Mac, the alert dialog will be a "sheet" on 
other platforms it has no effect. 

The type can be one of InfoAlert, WarningAlert, or ErrorAlert.

The title and message are displayed on the dialog box in a platform specific way.

One or more buttons can be defined. The button0 must be defined (non-nil). The
other buttons (button1 and button2) can be nil to hide them.

The function will return when one of the three buttons is pressed by the user.
The number of the button pressed is returned.
"""

watchFile """
The file or directory filename is watched (or not watched) by the application. 

If a change occurs a "file-changed" event will be sent to the user interface.  
In order to get notification of the file changing it is therefor necessary to
bind an event function to "file-changed" to watch for it. The Event.contents()
will give the name of the changed file.
"""

showConsole "Show the console window"

isConsoleVisible "Returns true if the console window is visible."

showTopViewToolbar "Show/Hide top view toolbar"
showBottomViewToolbar "Show/Hide bottom view toolbar"
isTopViewToolbarVisible "Returns true if the toolbar is visible"
isBottomViewToolbarVisible "Returns true if the toolbar is visible"

remoteSendMessage """
Send a message over the network to connected applications. The message
will be sent to each of the recipients. The recipients array should only
contain strings obtained from the remoteContacts() function.

remoteSendMessage() is most useful when RV is communicating with a non-RV
application or controller.
"""

remoteSendEvent """
Send an event message over the network to recipients. The event message
is a regular message but with an event name and target name attached. 
The recipients array should only contain strings obtained from the 
remoteContacts() function.

If the recipient is another RV process, it will send the event to the UI
as an application event with the contents as the Event.contents().
remoteSendEvent() is more useful than remoteSendMessage() when two RVs
are communicating.
"""

remoteSendDataEvent """
This function is similar to remoteSendEvent, but is used to send blobs
of binary data instead of plain text. In addition to the parameters it
shares with remoteSendEvent, this function also takes an interp string
(which could be e.g. a mime type) and a raw data parameter instead of
string contents.

If the recipient is another RV process, it will send the event to the UI
as an application event with the data as the Event.dataContents().
"""

remoteConnections """
Return an array of the remote connection names.
"""

remoteApplications """
Return an array of the remote application names."""

remoteContacts """
Returns an array of the remote contact names. These are typically 
in the form user@machine, but could also be the names of applications
like controller@machine.
"""

remoteLocalContactName """
This RV's contact name as seen by other connected applications."""

setRemoteLocalContactName """
Set this RV's contact name as seen by other connected applications.
If the network is active, this change will not take effect until the next
activation."""

remoteDefaultPermission """
The default permission this RV will use for new incoming connections.  One of
NetworkPermissionAsk, NetworkPermissionAllow, or NetworkPermissionDeny."""

setRemoteDefaultPermission """
Set the default permission this RV will use for new incoming connections.  One of
NetworkPermissionAsk, NetworkPermissionAllow, or NetworkPermissionDeny.
If the network is active, this change will not take effect until the next
activation."""

remoteConnect """
Initiate a remote connection to the contact name on the 
machine host at the given port. The port is optional.
"""

remoteDisconnect "Disconnect from remoteContact."

remoteNetwork "Turn networking on/off."

spoofConnectionStream """
When given the path of a file containing a stored RV connection stream, and a
"timeScale", the stored stream will be "replayed" by RV as if it was an
external connection.  The timeScale parameter can be used to scale the timing
of the spooffed connection stream; for example, a timeScale of 0.5 will play
back the connection in approximately half the time.  A timeScale of 1.0 will
play back the connection in roughly the time it took to record it.  A timeScale
of 0.0 will cause the stream to be replayed "as fast as possible".
"""

remoteNetworkStatus "Returns either NetworkStatusOn or NetworkStatusOff."

SettingsValue """
This is a union type used to supply data to the settings function
writeSetting() and readSetting(). The SettingsValue is defined as:

----
union: SettingsValue
{
    Int int
    | Float float
    | String string
    | Bool bool
    | IntArray int[]
    | FloatArray float[]
    | StringArray string[]
}
----

To create a SettingsValue its necessary to use one of the constructors:

----
let v = SettingsValue.Float(1.23);
----

To unpack a settings value use pattern matching:

----
let SettingsValue.Float f = SettingsValue.Float(1.23);
----

In this case "f" will be a float with the value 1.23.

You can reduce verbosity of code with "use":

----
use SettingsValue;
let v = Float(1.23);
let Float f = v;
assert(f == 1.23);
----

"""

readSetting """
Read a value from the user's settings (preferences) file. The function returns a 
SettingsValueType (a union) which must be unpacked. The group and name parameters
specify the full path to the settings value. The value parameters supplies a default
value and type to return if the setting doesn't exist in the file. The type of the 
setting value in the file must match the type of the default value passed in to
the function.

For example:

----
let SettingsValue.Bool b = 
        readSetting("mygroup", "foo", SettingsValue.Bool(false));
----

will search for a boolean setting value "foo" in "mygroup". If it does not exist,
the symbol "b" will have the value false. Otherwise it will have the value from the
settings file. There are a few possible types that can be passed to and returned
from readSetting:

----
SettingsValue.Float
SettingsValue.Int
SettingsValue.String
SettingsValue.Bool
SettingsValue.FloatArray
SettingsValue.IntArray
SettingsValue.StringArray
----

Another example with more types. In this example we've added "use SettingsValue"
to reduce the verbosity:

----
use SettingsValue;

let defaultSArray = string[] {"one", "two", "three"},
    defaultIArray = int[] {1, 2, 3},
    defaultFloat  = 3.141529;

let StringArray sarray = readSetting("mygroup", "one", StringArray(defaultSArrau)),
    IntArray iarray = readSetting("mygroup", "two", StringArray(defaultIArray)),
    Float f = readSetting("mygroup", "three", StringArray(defaultFloat));
----

Note that you need to use a "let" pattern matching expression to pull the
value out of a SettingsValue.

See also: writeSetting()
"""

writeSetting """
Write a value to the user's settings (preferences) file. The group and name parameters
specify the full path to the settings value. The value parameter will determine
both the value and type in the settings file.

For example:

----
writeSetting("mygroup", "foo", SettingsValue.Bool(true));
----

will write a boolean setting value to "foo" in "mygroup". SettingsValue types are limited 
to the following:

----
SettingsValue.Float
SettingsValue.Int
SettingsValue.String
SettingsValue.Bool
SettingsValue.FloatArray
SettingsValue.IntArray
SettingsValue.StringArray
----

Another example with more types. In this example we've added "use SettingsValue"
to reduce the verbosity:

----
use SettingsValue;
writeSetting("mygroup", "one", StringArray(string[] {"one", "two", "three"})),
writeSetting("mygroup", "two", IntArray(int[] {1,2,3})),
writeSetting("mygroup", "three", Float(3.141529));
writeSetting("mygroup", "four", Int(123));
----

See also: readSetting()
"""

httpGet """
Request a resource via http protocol. This function operates asynchronously. 

The function performs a GET on the server at the url with the provided headers.
When the server responds, RV will send events back to the UI to communicate
the results. The names of the call-back events are supplied when httpGet()
is called. 

The most imporant of the return events is the replyEvent which is called when
the data us fully collected. The data can be retrieved using the 
Event.dataContent() function. The type of data will be a mime type string which
can be retrieved with Event.contentType().

The optional authenticationEvent is sent if the resource requires authentication. This is
currently not supported.

The optional progressEvent is sent if partial data reads have occured.

The optional ignoreSslErrors is sent if an SSL error is encounted.

.Example
----
httpGet(...)
----

"""

httpPost """
Send data to a server at a url. This function operators asynchronously.

The function performs a POST on the server at the url with the provided headers.
When the server responds, RV will send events back to the UI to communicate
the results. The names of the call-back events are supplied when httpPost()
is called.

The data can be either a string or a byte[] for binary data.

.Example
----
httpPost(...)
----

"""

httpPut """
Send data to a server at a url. This function operators asynchronously.

The function performs a PUT on the server at the url with the provided headers.
When the server responds, RV will send events back to the UI to communicate
the results. The names of the call-back events are supplied when httpPut()
is called.

The data must be a byte[] for binary data.

.Example
----
httpPut(...)
----

"""

mainWindowWidget "Returns the main window Qt widget"
mainViewWidget "Returns a qt.QWidget corresponding to the session's QGLWidget"
prefTabWidget "Returns the preferences tab Qt widget"
sessionBottomToolBar "Returns the QToolBar for the current RV session bottom tool bar"
networkAccessManager "Returns Qt network access manager object"
//javascriptMuExport "Export Mu eval() call as a javascript Object in the given WebFrame"
sessionFromUrl "Create a session from a (possibly baked) rvlink URL"
putUrlOnClipboard "Copy a URL on the system clipboard"
getTextFromClipboard "Get the text sitting in your clipboard"
myNetworkPort "Returns the currently set port number for networking"
myNetworkHost "Returns the name of the host RV is running on for purposes of networking"
encodePassword "-"
decodePassword "-"
cacheDir "Location of http network object caching"
openUrl "Open the given URL using the prefered desktop application (browser, etc)"

existingFilesInSequence 
"""
Given a "sequence spec" like "foo.101-200#.jpg", returns a list of all files in the seuquence that actually
exist.
"""

readProfile """
Read and apply a profile (a baked description of a GroupNode and all its
contents). The profile can either be an absolute or relative path to a file or
a profile name in which case the profile path is searched for a file name like
"name.profile" or "name.tag.profile" if the tag argument is supplied.
"""

writeProfile """
Given a node write a Profile (a baked description of a GroupNode and all its
contents) for that node to a file.
"""

videoDeviceIDString """
For a device in a module return the ID string for one of the ModuleNameID, DeviceNameID, 
or VideoAndDataFormatID. This is currently used to locate settings for display profiles.
"""

}
}
