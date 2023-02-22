# Align Start Frames Package (to slide sources in time so that they can be compared)

This is an unsupported example that will probably be replaced with more complete tools in RV.

This package adds an "Align Start Frames" menu item to the the Edit menu.  This action offsets the start frame of all the loaded movies and sequences so that they match the start frame of the first source (the first loaded sequence or movie).

As you may be aware, RV preserves the time information represented by frame numbers. This has the advantage of placing sequences properly in time according to frame numbers. So in RV if you have a version of a shot foo.12-120#.exr and another version with different handles or cut points, foo_v2.18-122#.exr adn you load these into RV to do a wipe compare, then the frames will line up correctly in time. However, if you load a movie file (a quicktime) then it will start at frame 1 regardless of the source frames it represents. This package will let you compare a movie with the frames it came from by sliding all sources in time to line up on the same frame.

Similar results can also be accomplished on the command line using the range offset flag:

```
 rv [ foo.mov -ro 12 ] bar.12-120#.exr 
```
