# Chapter 3 - Command Line Usage

In this chapter, the emphasis will be on using the software from a Unix-like shell. On Windows, this can be done via Cygwin's bash shell or tcsh. If you choose to use the command.com shell, the command syntax will be the same, but some features (like pattern matching, etc) which are common with Unix shells may not work.

To use RV from a shell, you will need to have the binary executable in your path. On Linux and Windows the RV executable is located in the bin directory of the install tree.

On macOS this can be done by including /Applications/RV64.app/Contents/MacOS in your path (assuming you installed RV there). The executable on macOS is called RV64; you can either type the name in as is or create an alias or symbolic link from rv to RV.

There are a number of ways to start RV from the command line:

```
rv options
```

```
rv options source1 source2 source3 ...
```

```
rv options [ source1 source2 ... ] [ source4 source5 ... ]
```

```
rv options [ source-options source1 source2 ... ] [ source-options source4 source5 ... ]
```

```
rv file.rv
```

The command line options are not required and may appear throughout the command line. Sources are individual images, QuickTime .mov files, .avi files, audio files, image sequences or directory names. When specifying a .rv file, no other sources should be on the command line. (See Table [cap:RV-Command-Line](#per-source-command-line-options) )

The third example above uses square brackets around groups of sources. When sources appear between brackets they are called layers. These will be discussed in more detail below.

If RV is started with no arguments, it will launch a blank window; you can later add source material to the window via file browser or drag and drop.

Options are all preceded by a dash (minus sign) in the Unix tradition, even on windows. Some of them take arguments and some of them are flags which toggle the associated feature on or off. For example:

```
shell> rv -fullscreen foo.mov
```

plays back a movie file in full screen mode. In this case -fullscreen is a toggle which takes no arguments and foo.mov is one of the source material. If an option takes arguments, you supply them directly after the option:

```
shell> rv -fps 23.97 bar.#.exr
```

Here the -fps option (frames per second) requires a single floating point number

1

A floating point number in this context means a number which may or may not have a decimal point. E.g., 10 and 10.5 are both floating point numbers.

. With rare exception, RV's options are either toggles or take a single argument.gti

The most important option to remember is -help. The help option causes RV to print out all of the options and command line syntax and can be anywhere in a command line. When unsure of what the next argument is or whether you can add more options to a long command line, you can always add -help onto the end of your command and immediately hit enter. At that point, RV will ignore the entire command and print the help out. You can always use your shell's history to get the command back, remove the -help option, and continue typing the rest of the command.

```
shell> rv -fps 30 -fullscreen -l -lram .5 -help
(rv shows help)
Usage: RV movie and image viewer
...
(hit up arrow in shell, back up over -help and continue typing)
shell> rv -fps 30 -fullscreen -l -lram .5 -play foo.mov
```

Finally, you can do some simple arithmetic on option arguments. For example if you know you want to apply an inverse gamma of 2.2 to an image to view it you could do this:

```
shell> rv -gamma 1/2.2
```

which is identical to this:

```
shell> rv -gamma 0.454545454545
```

## Troubleshooting Open RV

Launching RV from the command line is the best way to troubleshoot RV by giving you access to error messages or crash dumps.

You can also access the logs at the following locations:

| OS | Logs location |
| --- | --- |
| Linux | ~/.local/share/ASWF/OpenRV/OpenRV.log |
| macOS | ~/Library/Logs/ASWF/OpenRV.log |
| Windows | %AppData%\Roaming\ASWF\OpenRV\OpenRV.log |

These logs contain anything that is written in the RV Console.

You can control the size and number of log files kept by RV with the following environment variables:

- `RV_FILE_LOG_SIZE`: Sets the maximum size of each log file. When that maximum size is reached, RV creates a new log file. Default value is 5 MB.
- `RV_FILE_LOG_NUM_FILES`: Sets the number of log files to keep. If there are a number of log files equal to `RV_FILE_LOG_NUM_FILES`, then RV deletes the oldest log file before creating a new one. Default value is 2.

### Command-Line Options

|                                   |                                                                                                                                                                                              |
| --------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| -c                                | Use region frame cache                                                                                                                                                                       |
| -l                                | Use look-ahead cache                                                                                                                                                                         |
| -nc                               | Use no caching                                                                                                                                                                               |
| -s float                          | Image scale reduction                                                                                                                                                                        |
| -stereo *string*                  | Stereo mode (hardware, checker, scanline, anaglyph, left, right, pair, mirror, hsqueezed, vsqueezed)                                                                                         |
| -vsync int                        | Video Sync (1 = on, 0 = off, default = 0)                                                                                                                                                    |
| -comp *string*                    | Composite mode (over, add, difference, replace, default=replace)                                                                                                                             |
| -layout *string*                  | Layout mode (packed, row, column, manual)                                                                                                                                                    |
| -over                             | Same as -comp over -view defaultStack                                                                                                                                                        |
| -diff                             | Same as -comp difference -view defaultStack                                                                                                                                                  |
| -tile                             | Same as -comp tile -view defaultStack                                                                                                                                                        |
| -wipe                             | Same as -over with wipes enabled                                                                                                                                                             |
| -view *string*                    | Start with a particular view                                                                                                                                                                 |
| -noSequence                       | Don't contract files into sequences                                                                                                                                                          |
| -inferSequence                    | Infer sequences from one file                                                                                                                                                                |
| -autoRetime *int*                 | Automatically retime conflicting media fps in sequences and stacks (1 = on, 0 = off, default = 1)                                                                                            |
| -rthreads int                     | Number of reader threads (default = 1)                                                                                                                                                       |
| -renderer *string*                | Default renderer type (Composite or Direct)                                                                                                                                                  |
| -fullscreen                       | Start in fullscreen mode                                                                                                                                                                     |
| -present                          | Start in presentation mode (using presentation device)                                                                                                                                       |
| -presentAudio int                 | Use presentation audio device in presentation mode (1 = on, 0 = off)                                                                                                                         |
| -presentDevice string             | Presentation mode device                                                                                                                                                                     |
| -presentVideoFormat string        | Presentation mode override video format (device specific)                                                                                                                                    |
| -presentDataFormat string         | Presentation mode override data format (device specific)                                                                                                                                     |
| -screen *int*                     | Start on screen (0, 1, 2, ...)                                                                                                                                                               |
| -noBorders                        | No window manager decorations                                                                                                                                                                |
| -geometry *int int *[_ int int _] | Start geometry x, y, w, h                                                                                                                                                                    |
| -init *string*                    | Override init script                                                                                                                                                                         |
| -nofloat                          | Turn off floating point by default                                                                                                                                                           |
| -maxbits *int*                    | Maximum default bit depth (default=32)                                                                                                                                                       |
| -gamma *float*                    | Set display gamma (default=1)                                                                                                                                                                |
| -sRGB                             | Display using linear -> sRGB conversion                                                                                                                                                      |
| -rec709                           | Display using linear -> Rec 709 conversion                                                                                                                                                   |
| -floatLUT *int*                   | Use floating point LUTs (requires hardware support, 1=yes, 0=no, default=_platform-dependant_)                                                                                               |
| -dlut *string*                    | Apply display LUT                                                                                                                                                                            |
| -brightness *float*               | Set display relative brightness in stops (default=0)                                                                                                                                         |
| -resampleMethod *string*          | Resampling method (area, linear, cube, nearest, default=area)                                                                                                                                |
| -eval *string*                    | Evaluate expression at every session start                                                                                                                                                   |
| -nomb                             | Hide menu bar on start up                                                                                                                                                                    |
| -play                             | Play on startup                                                                                                                                                                              |
| -fps float                        | Overall FPS                                                                                                                                                                                  |
| -cli                              | Mu command line interface                                                                                                                                                                    |
| -vram *float*                     | VRAM usage limit in Mb, default = 64.000000                                                                                                                                                  |
| -cram *float*                     | Max region cache RAM usage in Gb                                                                                                                                                             |
| -lram *float*                     | Max look-ahead cache RAM usage in Gb                                                                                                                                                         |
| -noPBO                            | Prevent use of GL PBOs for pixel transfer                                                                                                                                                    |
| -prefetch                         | Prefetch images for rendering                                                                                                                                                                |
| -bwait *float*                    | Max buffer wait time in cached seconds, default 5.0                                                                                                                                          |
| -lookback float                   | Percentage of the lookahead cache reserved for frames behind the playhead, default 25                                                                                                        |
| -yuv                              | Assume YUV hardware conversion                                                                                                                                                               |
| -volume float                     | Overall audio volume                                                                                                                                                                         |
| -noaudio                          | Turn off audio                                                                                                                                                                               |
| -audiofs *int*                    | Use fixed audio frame size (results are hardware dependant ... try 512)                                                                                                                      |
| -audioCachePacket *int*           | Audio cache packet size in samples (default=512)                                                                                                                                             |
| -audioMinCache *float*            | Audio cache min size in seconds (default=0.300000)                                                                                                                                           |
| -audioMaxCache *float*            | Audio cache max size in seconds (default=0.600000)                                                                                                                                           |
| -audioModule string               | Use specific audio module                                                                                                                                                                    |
| -audioDevice *int*                | Use specific audio device (default=-1)                                                                                                                                                       |
| -audioRate float                  | Use specific output audio rate (default=ask hardware)                                                                                                                                        |
| -audioPrecision int               | Use specific output audio precision (default=16)                                                                                                                                             |
| -audioNice *int*                  | Close audio device when not playing (may cause problems on some hardware) default=0                                                                                                          |
| -audioNoLock int                  | Do not use hardware audio/video synchronization (use software instead default=0)                                                                                                             |
| -audioGlobalOffset int            | Global audio offset in seconds                                                                                                                                                               |
| -bg string                        | Background pattern (default=black, grey18, grey50, checker, crosshatch)                                                                                                                      |
| -formats                          | Show all supported image and movie formats                                                                                                                                                   |
| -cmsTypes                         | Show all available Color Management Systems                                                                                                                                                  |
| -debug *string*                   | Debug category                                                                                                                                                                               |
| -cinalt                           | Use alternate Cineon/DPX readers                                                                                                                                                             |
| -exrcpus *int*                    | EXR thread count (default=2)                                                                                                                                                                 |
| -exrRGBA                          | EXR use basic RGBA interface (default=false)                                                                                                                                                 |
| -exrInherit                       | EXR guesses channel inheritance (default=false)                                                                                                                                              |
| -exrIOMethod int [int]            | EXR I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=0) and optional chunk size (default=61440)                                    |
| -jpegRGBA                         | Make JPEG four channel RGBA on read (default=no, use RGB or YUV)                                                                                                                             |
| -jpegIOMethod int [int]           | JPEG I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=0) and optional chunk size (default=61440)                                   |
| -cinpixel *string*                | Cineon/DPX pixel storage (default=RGB8_PLANAR)                                                                                                                                               |
| -cinchroma                        | Cineon pixel storage (default=RGB8_PLANAR)                                                                                                                                                   |
| -cinIOMethod int [int]            | ineon I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=3) and optional chunk size (default=61440)                                  |
| -dpxpixel string                  | DPX pixel storage (default=RGB8_PLANAR)                                                                                                                                                      |
| -dpxchroma                        | Use DPX chromaticity values (for default reader only)                                                                                                                                        |
| -dpxIOMethod int [int]            | DPX I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=3) and optional chunk size (default=61440)                                    |
| -tgaIOMethod int [int]            | TARGA I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=2) and optional chunk size (default=61440)                                  |
| -tiffIOMethod int [int]           | TIFF I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=2) and optional chunk size (default=61440)                                   |
| -noPrefs                          | Ignore preferences                                                                                                                                                                           |
| -resetPrefs                       | Reset preferences to default values                                                                                                                                                          |
| -qtcss *string*                   | Use QT style sheet for UI                                                                                                                                                                    |
| -qtstyle *string*                 | Use QT style, default=""                                                                                                                                                                     |
| -qtdesktop                        | QT desktop aware, default=1 (on)                                                                                                                                                             |
| -xl                               | Aggressively absorb screen space for large media                                                                                                                                             |
| -mouse int                        | Force tablet/stylus events to be treated as a mouse events, default=0 (off)                                                                                                                  |
| -network                          | Start networking                                                                                                                                                                             |
| -networkPort int                  | Port for networking                                                                                                                                                                          |
| -networkHost *string*             | Alternate host/address for incoming connections                                                                                                                                              |
| -networkConnect *string *[int]    | Start networking and connect to host at port                                                                                                                                                 |
| -networkPerm int                  | Default network connection permission (0=Ask, 1=Allow, 2=Deny, default=0)                                                                                                                    |
| -reuse int                        | Try to re-use the current session for incoming URLs (1 = reuse session, 0 = new session, default = 1; macOS only)                                                                             |
| -nopackages                       | Don't load any packages at startup (for debugging)                                                                                                                                           |
| -encodeURL                        | Encode the command line as an rvlink URL, print, and exit                                                                                                                                    |
| -bakeURL                          | Fully bake the command line as an rvlink URL, print, and exit                                                                                                                                |
| -flags *string*                   | Arbitrary flags (flag, or 'name=value') for use in Mu code                                                                                                                                   |
| -prefsPath *string*               | Alternate path to preferences directory                                                                                                                                                      |
| -registerHandler                  | Register this executable as the default rvlink protocol handler (macOS only)                                                                                                                  |
| -scheduler *string*               | Thread scheduling policy (may require root, Linux only)                                                                                                                                      |
| -priorities int int               | Set display and audio thread priorities (may require root, Linux only)                                                                                                                       |
| -version                          | Show RV version number                                                                                                                                                                       |
| -pa float                         | Set the Pixel Aspec Ratio                                                                                                                                                                    |
| -ro int                           | Shifts first and last frames in the source range (range offset)                                                                                                                              |
| -rs int                           | Sets first frame number to argument and offsets the last frame number                                                                                                                        |
| -fps float                        | FPS override                                                                                                                                                                                 |
| -ao float                         | Audio Offset. Shifts audio in seconds (audio offset)                                                                                                                                         |
| -so float                         | Set the Stereo Eye Relative Offset                                                                                                                                                           |
| -volume float                     | Audio volume override (default = 1)                                                                                                                                                          |
| -fcdl filename                    | Associate a file CDL with the source                                                                                                                                                         |
| -lcdl filename                    | Associate a look CDL with the source                                                                                                                                                         |
| -flut filename                    | Associate a file LUT with the source                                                                                                                                                         |
| -llut filename                    | Associate a look LUT with the source                                                                                                                                                         |
| -pclut *filename*                 | Associate a pre-cache software LUT with the source                                                                                                                                           |
| -cmap *channels*                  | Remap color channels for this source (channel names separated by commas)                                                                                                                     |
| -select *selectType selectName*   | Restrict loaded channels to a single view/layer/channel.* selectType* must be one of view, layer, or channel.* selectName* is a comma-separated list of view name, layer name, channel name. |
| -crop x0 y0 x1 y1                 | Crop image to box (all integer arguments)                                                                                                                                                    |
| -uncrop width height x y          | Inset image into larger virtual image (all integer arguments)                                                                                                                                |
| -in int                           | Cut-in frame for this source in default EDL                                                                                                                                                  |
| -out int                          | Cut-out frame for this source in default EDL                                                                                                                                                 |
| -noMovieAudio                     | Turn off source movie's baked-in audio (aka “-nma”)                                                                                                                                          |

Table 3.1: Per-Source Command Line Options <a id="per-source-command-line-options"></a>

## 3.1 Image Sequence Notation

RV has a special syntax to describe image sequences as source movies. Sequences are assumed to be files with a common base name followed by a frame number and an image type extension. For example, the filenames foo.1.tif and foo.0001.tif would be interpreted as frame 1 of the TIFF sequence foo. RV sorts images by frame numbers in numeric order. It sorts image base names in lexical order. What this means is that RV will sort images into sequences the way you expect it to. Padding tricks are unnecessary for RV to get the image order correct; image order will be interpreted correctly.

```sh
foo.0001.tif foo.0002.tif foo.0003.tif foo.0004.tif
foo.0005.tif foo.0006.tif foo.0007.tif foo.0008.tif
foo.0009.tif foo.0010.tif
```

To play this image sequence in RV from the command line, you could start RV like this:

```sh
shell> rv foo.\*.tif
```

and RV will automatically attempt to group the files into a logical movie. ( **Note** : this will only work on Linux or Mac OS, or some other Unix-like shell, like cygwin on Windows.)

When you want to play a subset of frames or audio needs to be included, you can specify the sequence using the \`\`#'' or \`\`@'' notation (similar to Shake's) or the printf-like notation using \`\`%'' similar to Nuke.

```sh
rv foo.#.tif
rv foo.2-8#.tif
rv foo.2-8@@@@.tif
rv foo.%04d.tif
rv foo.%04d.tif 2-8
rv foo.#.tif 2-8
```

The first example above plays all frames in the foo sequence, the second line plays frames starting at frame 2 through frame 8. The third line uses the \`\`@'' notation which forces RV to assume 0 padded frame numbers–in this case, four \`\`@'' characters indicate a four character padding.

The next two examples use the printf-like syntax accepted by nuke. In the first case, the entire frame range is specified with the assumption that the frame numbers will be padded with 0 up to four characters (this notation will also work with 6 or other amounts of padding). In the final two examples, the range is limited to frames 2 through 8, and the range is passed as a separate argument.

Sometimes, you will encounter or create an image sequence which is purposefully missing frames. For example, you may be rendering something that is very expensive to render and only wish to look at every tenth frame. In this case, you can specify the increment using the \`\`x'' character like this:

```sh
rv foo.1-100x10#.tif
```

or alternately like this using the \`\`@'' notation for padding to four digits:

```sh
rv foo.1-100x10@@@@.tif
```

or if the file was padded to three digits like foo.001.tif:

```sh
rv foo.1-100x10@@@.tif
```

In these examples, RV will play frames 1 through 100 by tens. So it will expect frames 1, 11, 21, 31, 41, on up to 91.

If there is no obvious increment, but the frames need to be group into a sequence, you can list the frame numbers with commas:

```sh
rv foo.1,3,5,7,8,9#.tif
```

In many cases, RV can detect file types automatically even if a file extension is not present or is mis-labeled.

**NOTE** : Use the same format for exporting multiple annotated frames.

### 3.1.1 Negative Frame Numbers

RV can handle negative frames in image sequences. If the frame numbers are zero padded, they should look like so:

```sh
foo.-012.tif
foo.-001.tif
```

To specify in and out points on the command line in the presence of negative frames, just include the minus signs:

```sh
foo.-10-20#.tif
foo.-10--5#.tif
```

The first example uses frames -10 to +20. The second example uses frames -10 to -5. Although the use of the \`\`-'' character to specify ranges can make the sequence a bit visually confusing, the interpretation is not ambiguous.

### 3.1.2 Stereo Notation

RV can accept stereo notation similar to Nuke's \`\`%v'' and \`\`%V'' syntax. By default, RV can only recognize left, right, Left, and Right for %V and for %v it will try L, R, or l and r. You can change the substitutions by setting the environment variables RV_STEREO_NAME_PAIRS and RV_STEREO_CHAR_PAIRS. These should be set to a colon separated list of values (even on windows). For example, the defaults would look like this:

```sh
RV_STEREO_NAME_PAIRS = left:right:Left:Right
RV_STEREO_CHAR_PAIRS = L:R:l:r
```

So for example, if you have two image sequences:

```sh
foo.0001.left.exr
foo.0002.left.exr
foo.0001.right.exr
foo.0002.right.exr
```

you could refer to the entire stereo sequence as:

```sh
foo.%04d.%V.exr
```

## 3.2 Source Layers from the Command Line

You can create source material from multiple audio, image, and movie files from the command line by enclosing them in square brackets. The typical usage looks something like this:

```sh
shell> rv [ foo.#.exr soundtrack.aiff ]
```

Note that there are spaces around the brackets: they must be completely separated from subsequent arguments to be interpreted correctly. You cannot nest brackets and the brackets must be matched; for every begin bracket there must be an end bracket.

### 3.2.1 Associating Audio with Image Sequences or Movie Files

Frequently a movie file or image sequence needs to be viewed with one or more separate audio files. When you have multiple layers on the command line and one or more of the layers are audio files, RV will play back the all of the audio files mixed together along with the images or movies. For example, to play back a two wav files with an image sequence:

```sh
shell> rv [ foo.#.exr first.wav second.wav ]
```

If you have a movie file which already has audio you can still add additional audio files to be played:

```sh
shell> rv [ movie_with_audio.mov more_audio.aiff ]
```

### 3.2.2 Dual Image Sequences and/or Movie Files as Stereo

It's not unusual to render left and right eyes separately and want to view them as stereo together. When you give RV multiple layers of movie files or image sequences, it uses the first two as the left and right eyes.

```sh
shell> rv [ left.#.exr right.#.exr ]
```

It's OK to mix and match formats with layers:

```sh
shell> rv [ left.mov right.100-300#.jpg ]
```

if you want audio too:

```sh
shell> rv [ left.#.exr right.#.exr soundtrack.aiff ]
```

As with the mono case, any number of audio files can be added: they will be played simultaneously.

### 3.2.3 Per-Source Arguments

There are a few arguments which can be applied with in the square brackets (See Table [3.1](#per-source-command-line-options) ). The range start sets the first frame number to its argument; so for example to set the start frame of a movie file with or without a time code track so that it starts at frame 101:

```sh
shell> rv [ -rs 101 foo.mov ]
```

You must use the square brackets to set per-source arguments (and the square brackets must be surrounded by spaces).

The -in and -out per-source arguments are an easy way to create an EDL on the command line, even when playing movie files.

### 3.2.4 A note on the -fps per-source argument

The point of the -fps arg is to provide a scaling factor in cases where the frame rate of the media cannot be determined and you want to play an audio file with it. For example, if you want to play "[foo.#.dpx foo.wav]" in the same session with "[bar.#.dpx bar.wav]" but the "native frame rate of foo is 24fps and the native frame rate of bar is 30fps, then you might want to say:

```sh
shell> rv [foo.#.dpx foo.wav -fps 24 ] [ bar.#.dpx bar.wav -fps 30 ]
```

This will ensure that the video and audio are synced properly no matter what frame rate you use for playback. To clarify further, the per-source -fps flag has no relation to the frame rate that is used for playback, and in general RV plays media (all loaded media) at whatever single frame rate is currently in use.

### 3.2.5 Source Layer Caveats and Capabilities

There are a number of things you should be aware of when using source layers. In most cases, RV will attempt to do something with what you give it. However, if your input is logically ambiguous the result may be different than what you expect. Here are some things you should avoid using in layers of a single source:

- Images or movies with differing frame rates
- Images or movies with different image or pixel aspect ratios
- Images or movies which require special color correction for only one eye
- Images or movies stored with differing color spaces (e.g. cineon log images + jpeg)

Here are some things that are OK to do with layers:

- Images with different resolutions but the same image aspect ratio
- Images with different bit depths or number of channels or chroma sampling
- Audio files with different sample rates or bit depths
- Mixing movies with audio and separate audio files
- An image sequence for one eye and a movie for the other
- A movie with audio for one eye and a movie without audio for the other
- Audio files with no imagery

## 3.3 Directories as Input

If you give RV the name of a directory instead of a single file or an image sequence it will attempt to interpret the contents of the directory. RV will find any single images, image sequences, or single movie files that it can and present them as individual source movies. This is especially useful with the directory \`\`.'' on Linux and macOS. If you navigate in a shell to a directory that contains an image sequence for example, you need only type the following to play it:

```sh
rv .
```

You don't even need to get a directory listing. If RV finds multiple sequences or a sequence and movie files, it will sort and organize them into a playlist automatically. RV will attempt to read files without extensions if they look like image files (for example the file ends in a number). If RV is unable to parse the contents of a directory correctly, you will need to specify the image sequences directly to force it to read them.
