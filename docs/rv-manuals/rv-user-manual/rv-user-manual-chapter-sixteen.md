# Chapter 16 - RVIO

RVIO is a command line (re)mastering tool–it converts image sequence, audio files, and movie files from one format into another including possible bit depth and image color and size resolution changes. RVIO can also generate custom slate frames, add per-frame information and matting directly onto output images, and change color spaces (to a limited extent).

RVIO supports all of the same movie, image, and audio formats that RV does including the .rv session file. RV session files (.rv) can be used to specify compositing operations, split screens, tiling, color corrections, pixel aspect ratio changes, etc.

*Table 16.1: rvio Options*

|                           |                                                                                                                                                              |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| -o *string*               | Output sequence or image                                                                                                                                     |
| -t *string*               | Output time range (default=input time range)                                                                                                                 |
| -tio                      | Output time range from rv session files in/out points                                                                                                        |
| -v                        | Verbose messages                                                                                                                                             |
| -vv                       | Really Verbose messages                                                                                                                                      |
| -q                        | Best quality for color conversions (slower – mostly unnecessary)                                                                                             |
| -noRanges                 | No separate frame ranges (1-10 will be considered a file)                                                                                                    |
| -rthreads *int*           | Number of reader/render threads (default=1)                                                                                                                  |
| -wthreads *int*           | Number of writer threads (default=same as -rthreads)                                                                                                         |
| -formats                  | Show all supported image and movie formats                                                                                                                   |
| -iomethod *int* [*int*]   | I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=3) and optional chunk size (default=61440)        |
| -view *string*            | View to render (default=defaultSequence or current view in rv file)                                                                                          |
| -leader ...               | Insert leader/slate (can use multiple time)                                                                                                                  |
| -leaderframes *int*       | Number of leader frames (default=1)                                                                                                                          |
| -overlay ...              | Visual overlay(s) (can use multiple times)                                                                                                                   |
| -inlog                    | Convert input to linear space via Cineon Log->Lin                                                                                                            |
| -insrgb                   | Convert input to linear space from sRGB space                                                                                                                |
| -in709                    | Convert input to linear space from Rec-709 space                                                                                                             |
| -ingamma *float*          | Convert input using gamma correction                                                                                                                         |
| -fileGamma *float*        | Convert input to linear space using gamma correction                                                                                                         |
| -inchannepmap ...         | map input channels                                                                                                                                           |
| -inpremult                | premultiply alpha and color                                                                                                                                  |
| -inunpremult              | un-premultiply alpha and color                                                                                                                               |
| -exposure *float*         | Apply relative exposure change (in stops)                                                                                                                    |
| -scale *float*            | Scale input image geometry                                                                                                                                   |
| -resize *int* [*int*]     | Resize input image geometry to exact size on input (0 = maintain image aspect)                                                                               |
| -resampleMethod *string*  | Resampling method (area, linear, cubic, nearest, default=area)                                                                                               |
| -floatLUT *int*           | Use floating point LUTs (1=yes, 0=no, default=1)                                                                                                             |
| -flut *string*            | Apply file LUT                                                                                                                                               |
| -dlut *string*            | Apply display LUT                                                                                                                                            |
| -flip                     | Flip image (flip vertical) (keep orientation flags the same)                                                                                                 |
| -flop                     | Flop image (flip horizontal) (keep orientation flags the same)                                                                                               |
| -yryby *int int int*      | Y RY BY sub-sampled planar output                                                                                                                            |
| -yrybya *int int int int* | Y RY BY A sub-sampled planar output                                                                                                                          |
| -yuv *int int int*        | Y U V sub-sampled planar output                                                                                                                              |
| -outparams ...            | Codec specific output parameters                                                                                                                             |
| -outchannelmap ...        | map output channels                                                                                                                                          |
| -outrgb                   | same as -outChannelMap R G B                                                                                                                                 |
| -outpremult               | premultiply alpha and color                                                                                                                                  |
| -outunpremult             | un-premultiply alpha and color                                                                                                                               |
| -outlog                   | Convert output to log space via Cineon Lin->Log                                                                                                              |
| -outsrgb                  | Convert output to sRGB ColorSpace                                                                                                                            |
| -out709                   | Convert output to Rec-709 ColorSpace                                                                                                                         |
| -outgamma                 | Apply gamma to output                                                                                                                                        |
| -outstereo *string*       | Output stereo (checker, scanline, anaglyph, lumanaglyph, left, right, pair, mirror, hsqueezed, vsqueezed, default=separate)                                  |
| -outformat *int string*   | Output bits and format (e.g. 16 float -or- 8 int)                                                                                                            |
| -outhalf                  | Same as -outformat 16 float                                                                                                                                  |
| -out8                     | Same as -outformat 8 int                                                                                                                                     |
| -outres *int int*         | Output resolution (image will be fit, not stretched)                                                                                                         |
| -outfps                   | Output FPS                                                                                                                                                   |
| -codec *string*           | Output codec (varies with file format)                                                                                                                       |
| -audiocodec *string*      | Output audio codec (varies with file format)                                                                                                                 |
| -audiorate *float*        | Output audio sample rate (default from input)                                                                                                                |
| -audiochannels *int*      | Output audio channels (default from input)                                                                                                                   |
| -quality *float*          | Output codec quality 0.0 -> 1.0 (use varies with file format and codec default=0.900000)                                                                     |
| -outpa*float*             | Output pixel aspect ratio (e.g. 1.33 or 4:3, etc, metadata only) default=1:1                                                                                 |
| -comment *string*         | Output comment (movie files, default="")                                                                                                                     |
| -copyright *string*       | Output copyright (movie files, default="")                                                                                                                   |
| -debug *string*           | Debug category                                                                                                                                               |
| -version                  | Show RVIO version number                                                                                                                                     |
| -exrcpus *int*            | EXR thread count (default=*platform dependant*)                                                                                                              |
| -exrRGBA                  | EXR use basic RGBA interface (default=false)                                                                                                                 |
| -exrInherit               | EXR guesses channel inheritance (default=false)                                                                                                              |
| -exrIOMethod int [int]    | EXR I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=0) and optional chunk size (default=61440)    |
| -jpegRGBA                 | Make JPEG four channel RGBA on read (default=no, use RGB or YUV)                                                                                             |
| -jpegIOMethod int [int]   | JPEG I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=0) and optional chunk size (default=61440)   |
| -cinpixel *string*        | Cineon/DPX pixel storage (default=RGB16)                                                                                                                     |
| -cinchroma                | Use file chromaticity values (ignores them by default)                                                                                                       |
| -cinIOMethod int [int]    | Cineon I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=3) and optional chunk size (default=61440) |
| -dpxpixel string          | DPX pixel storage (default=RGB16)                                                                                                                            |
| -dpxchroma                | Use DPX chromaticity values (ignores them by default)                                                                                                        |
| -dpxIOMethod int [int]    | DPX I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=3) and optional chunk size (default=61440)    |
| -tgaIOMethod int [int]    | TARGA I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=2) and optional chunk size (default=61440)  |
| -tiffIOMethod int [int]   | TIFF I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=2) and optional chunk size (default=61440)   |
| -init *string*            | Override init script                                                                                                                                         |
| -err-to-out               | Output errors to standard output (instead of standard error)                                                                                                 |
| -noprerender              | Turn off prerendering optimization                                                                                                                           |
| -flags ...                | Arbitrary flags (flag, or 'name=value') for Mu or Python                                                                                                     |

### 16.1 Basic Usage

#### 16.1.1 Image Sequence Format Conversion

RVIO's most basic operation is to convert a sequence of images from one format into another. RVIO uses the same sequence notation as RV to specify input and output sequences. For example:

```
shell> rvio foo.#.exr -o foo.#.tif
shell> rvio foo.#.exr -o foo.#.jpg
```

#### 16.1.2 Image sequence to QuickTime Movie Conversion

RVIO can write out QuickTime movies. The default compression codec is PhotoJPEG with a default quality of 0.9.

```
shell> rvio foo.#.exr -o foo.mov
```

#### 16.1.3 Resizing and Scaling

Rvio does high quality filtering when it resizes images. Output resolution can be specified explicitly, in which case RVIO will conform to the new resolution, padding the image to preserve pixel aspect ratio.

```
shell> rvio foo.1-100@@@@.tga -scale 0.5 -o foo.#.tga
shell> rvio foo.#.exr -outres 640 480
```

Or you can use the -resize x y option, in which case scaling will occur in either dimension (unless the number specified for that dimension is 0).

#### 16.1.4 Adding Audio to Movies

RVIO uses the same layer notation as RV to combine audio with image sequences. Multiple audio sources can be mixed in a single layer.

```
shell> rvio [ foo.1-300#.tif foo.aiff ] -o foo.mov
shell> rvio [ foo.1-300#.tif foo.aiff bar.aiff ] -o foo.mov
```

### 16.2 Advanced Usage

#### 16.2.1 Editing Sequences

RVIO can combine multiple sequences and write out a single output sequence or movie. This allows you to quickly edit and conform.

```
shell> rvio foo.25-122#.exr bar.8-78#.exr -o foo.mov
shell> rvio [ foo.#.exr foo.aiff ] [ bar.#.exr bar.aiff ] \
       -o foobar.mov
```

Note that you can cut in and out of movie files as well:

```
shell> rvio [ foo.mov -in 101 -out 120 ] [ bar.mov -in 58 -out 123 ] -o out.mov
```

#### 16.2.2 Processing Open RV Session Files

RV session files can be used as a way to author operations for processing with RVIO. Most operations and settings in RV can be used by RVIO. For example RV can be used in a compositing mode to do an over or difference or split-screen comparison. RV can also be used to set up edits with per source color corrections (e.g. a unique LUT for each sequences, exposure or color adjustments per sequences, an overall output LUT, etc.).

```
shell> rvio foo.rv -o foo.mov
```

Any View in the session file can be used as the source for the RVIO output:

```
shell> rvio foo.rv -o foo.mov -view "latest shots"
```

#### 16.2.3 Advanced Image Conversions

RVIO has flags to handle standard colorspace, gamma, and log/lin conversions for both inputs and outputs

```
shell> rvio foo.#.cin -inlog -o foo.#.exr
shell> rvio foo.#.exr -o foo.#.cin -outlog
shell> rvio foo.#.jpeg -insrgb -o foo.#.exr
shell> rvio foo.#.exr -o foo.#.tiff -outgamma 2.2
shell> rvio foo.#.exr -o foo.#.jpeg -outsrgb
shell> rvio foo.#.cin -inlog -o foo.mov -outsrgb \
       -comment "Movie is in sRGB space"
```

#### 16.2.4 LUTs

RVIO can apply a LUT to input files and an output LUT. RVIO's command line only supports one file LUT and one display LUT. The file LUT will be applied to all the input sources before conversion and and the output LUT will be applied to the entire session on output. If you need to process a sequences with a different file LUT per sequence, you can do that by creating an RV session file with the desired LUTs and color settings to use as input to RVIO.

```
shell> rvio foo.#.cin bar.#.cin -inlog -flut in.cube \
       -dlut out.cube -o foobar.mov
```

#### 16.2.5 Pixel Storage Formats and Channel Mapping

RVIO provides control over pixel storage (floating point or integer, bit depth, planar subsampling) and channel mapping. The planar subsampling options are particularly used to support OpenExr's B44 compression, which is a fixed bandwidth, floating point, high-dynamic range compression scheme.

```
shell> rvio foo.#.exr -outformat 8 int -o foo.#.tif
shell> rvio foo.#.exr -outformat 8 int -o foo.#.tif -outrgb
shell> rvio foo.#.cin -inlog -o foo..#.tiff -outformat 16 float
shell> rvio foo.#.exr -outformat 32 float -o foo.#.tif
shell> rvio foo.#.exr -codec B44A  -yrybya 1 2 2 2 -outformat \
       16 float
shell> rvio foo.#.exr -codec B44A  -yryby 1 2 2  -outformat \
       16 float -outchannelmap R G B
```

#### 16.2.6 Advanced QuickTime Movie Conversions

RVIO uses ffmpeg for reading and writing. You can find out what codecs are available for reading and writing by using the "-formats" flag. This will also tell you the full name of the encoder, e.g. writing Motion JPEG requires "mjpeg" by ffmpeg. RVIO lets you specify the output codec and can collect parameters for the output from "outparams". Please examine the following two examples. The first is with the default settings for mjpeg's Motion JPEG writing, and the second is a popular command derived from using the ffmpeg binary directly. Something of the form `ffmpeg -i foo.%04d.tif -ac 2 -b:v 2000k -c:a pcm_s16be -c:v mjpeg -pix_fmt yuv420p -g 30 -b:a 160k -vprofile high -bf 0 -strict experimental -f mov outfile.mov`.

```
shell> rvio foo.#.tif -codec mjpeg -o foo.mov
shell> rvio foo.#.tif -codec mjpeg -audiocodec pcm_s16be \
       -outparams pix_fmt=yuv420p vcc:b=2000000 acc:b=160000 \
        vcc:g=30 vcc:profile=high vcc:bf=0 -o foo.mov
```

#### 16.2.7 Audio Conversions

RVIO can operate directly on audio files and can also add or extract audio to/from QuickTime movies. RVIO provides flags to set the audio codec, sample rate, storage format (8, 16, and 32 bits) storage type (int, float), and number of output channels. RVIO does high quality resampling using 32 bit floating point operations. RVIO will mix together multiple audio files specified in a layer.

```
shell> rvio foo.mov -o foo.aiff
shell> rvio foo.mov -o foo.aiff -audiorate 22050
shell> rvio [ foo.#.exr foo.aiff bar.wav ] -codec H.264 \
       -quality  1.0 \
       -audiocodec "Apple Lossless" -audiorate 44100 \
       -audioquality 1.0 -o foo.mov
```

#### 16.2.8 Stereoscopic and Multiview Conversions

RV and RVIO support stereoscopic playback and conversions. RVIO can be used to create stereo QuickTime files or multiview OpenExr files (Weta's SXR files) by specifying two input layers and using the -outstereo flag. Stereo QuickTime movies contain multiple video tracks. RV interprets the fist two tracks as left and right views. RVIO will also render output using stereo modes specified in an RV session file–this allows you to output anaglyph images from stereo inputs or to render out scanline or alternating pixel stereo material.

```
shell> rvio [ foo_l.#.exr foo_r.#.exr foo.aiff ] -outstereo \
       -o stereo.mov
shell> rvio [ foo_left.#.exr foo_right.#.exr ] -outstereo \
       -o stereo.#.exr
```

Or you can specify an output stereo format:

```
shell> rvio [ foo_l.#.exr foo_r.#.exr foo.aiff ] -outstereo hsqueezed \
       -o stereo.mov
```

#### 16.2.9 Slates, Mattes, Watermarks, and Burn-ins

RVIO supports script based creation of slates and overlays. The default scripts that come with RV can be used as is, or they can be customized to create any kind of overlay or slate that you need. Customization of these scripts is covered in the RV Reference Manual. RVIO has two command line flags to manage these scripts, -leader and -overlay. -leader scripts are used to create Slates or other frames that will be added to the beginning of a sequence or movie. -overlay scripts will draw on top of the image frames. Multiple overlays can be layered on top of the image, so that you can build up a frame with mattes, frame burn-in, bugs, etc. The scripts that come with RV include:

- simpleslate
- watermark
- matte
- frameburn
- bug

##### Simpleslate Leader

Simpleslate allows you to build up a slate from a list of attribute/value pairs. It will automatically scale all of your text to fit onto the frame. It works like this:

```
shell> rvio foo.#.exr -leader simpleslate \
       "Acme Post" "Show=My Show" "Shot=foo" "Type=comp" \
       "Artist=John Doe" "Comments=Lighter and Darker as \
       requested by director" -o foo.mov
```

##### Watermark Overlay

The watermark overlay burns a text comment onto output images. This makes it easy to generate custom watermarked movies for clients or vendors. Watermark takes two arguments, the comment in quotes and the opacity (from 0 to 1).

```
shell> rvio foo.mov -overlay watermark "For Client X Review" 0.1 \
       -o foo_client_x.mov
```

##### Matte Overlay

The matte overlay mattes your images to the desired aspect ratio. It takes two arguments, the aspect ratio and the opacity.

```
shell> rvio foo.#.exr -overlay matte 2.35 1.0 -o foo.mov
```

##### Frameburn Overlay

The frameburn overlay renders the source frame number onto each frame. It takes three arguments, the opacity, the luminance, and the size.

```
shell> rvio foo.#.exr -overlay frameburn 0.1 0.1 50 -o foo.mov
```

##### Bug Overlay

The bug overlay lets you render an image o top of each output frame. It takes three arguments, the image name, the opacity, and the size

```
shell> rvio foo.#.exr -overlay bug "/path/to/logo.tif" 0.5 48
```

#### 16.2.10 EXR Attributes

RVIO can create and pass through header attributes. To create an attribute from the command line use -outparams:

```
shell> rvio in.exr -o foo.exr -outparams NAME:TYPE=VALUE0,VALUE1,VALUE2,...
```

TYPE is one of:

|      |                     |
| ---- | ------------------- |
| f    | float               |
| i    | int                 |
| s    | string              |
| sv   | Imf::StringVector   |
| v2i  | Imath::V2i          |
| v2f  | Imath::V2f          |
| v3i  | Imath::V3i          |
| v3f  | Imath::V3f          |
| b2i  | Imath::Box2i        |
| b2f  | Imath::Box2f        |
| m33f | Imath::M33f         |
| m44f | Imath::M44f         |
| c    | Imf::Chromaticities |

Values are comma separated. For example to create a Imath::V2i attribute called myvec with the value V2i(1,2):

```
shell> rvio in.exr -o out.exr -outparams myvec:v2i=1,2
```

similarily a string vector attribute would be:

```
shell> rvio in.exr -o out.exr -outparams mystringvector:sv=one,two,three,four
```

and a 3 by 3 float matrix attribute would be:

```
shell> rvio in.exr -o out.exr -outparams myfloatmatrix:m33f=1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0
```

where the first row of the matrix would be 1.0 2.0 3.0.

If you want to pass through attributes from the incoming image to the output EXR file you can use the passthrough variable. Setting passthrough to a regular expression will cause the writer code to select matching incoming attribute names.

```
shell> rvio in.exr -o out.exr -outparams "passthrough=.*"
```

The name matching includes any non-EXR format identifiers that are created by RV and RVIO.

```
shell> rvio in.jpg -o out.exr -insrgb -outparams "passthrough=.*EXIF.*"
```

The name matching includes any EXIF attributes (e.g. from TIFF or JPEG files).

#### 16.2.11 IIF/ACES Files

RVIO can convert pixels to the very wide gamut ACES color space for output using the -outaces flag. In addition, use of the .aces extension will cause the OpenEXR writer to enforce the IIF container subset of EXR. For example, to convert an existing EXR file to an IIF file:

```
shell> rvio in.exr -o out.aces -outaces
```

It's possible to write to other formats using -outaces, but it's not recommended.

#### 16.2.12 DPX Header Fields

Using -outparams it's possible to set almost any DPX header field. Setting the field will not change the pixels in the final file, just the value of the header field.

There are a couple of fields treated in a special way: the frame_position, tv/time_code, and tv/user_bits fields are all increment automatically when a sequence of frames is output. In the case of tv/user_bits and tv/time_code, the initial value comes either from the output frame number, or starting at the time code passed in.

*Table 16.2: DPX Output Parameters*

| Keyword                   | Description                                                                                                                                                 |
| ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| transfer                  | Transfer function (LOG, DENSITY, REC709, USER, VIDEO, SMPTE274M, REC601-625, REC601-525, NTSC, PAL, or number)                                              |
| colorimetric              | Colorimetric specification (REC709, USER, VIDEO, SMPTE274M, REC601-625, REC601-525, NTSC, PAL, or number)                                                   |
| creator                   | ASCII string                                                                                                                                                |
| copyright                 | ASCII string                                                                                                                                                |
| project                   | ASCII string                                                                                                                                                |
| orientation               | Pixel Origin string or int (TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, ROTATED_TOP_LEFT, ROTATED_TOP_RIGHT, ROTATED_BOTTOM_LEFT, ROTATED_BOTTOM_RIGHT) |
| create_time               | ISO 8601 ASCII string: YYYY:MM:DD:hh:mm:ssTZ                                                                                                                |
| film/mfg_id               | 2 digit manufacturer ID edge code                                                                                                                           |
| film/type                 | 2 digit film type edge code                                                                                                                                 |
| film/offset               | 2 digit film offset in perfs edge code                                                                                                                      |
| film/prefix               | 6 digit film prefix edge code                                                                                                                               |
| film/count                | 4 digit film count edge code                                                                                                                                |
| film/format               | 32 char film format (e.g. Academy)                                                                                                                          |
| film/frame_position       | Frame position in sequence (0 indexed)                                                                                                                      |
| film/sequence_len         | Sequence length                                                                                                                                             |
| film/frame_rate           | Frame rate (frames per second)                                                                                                                              |
| film/shutter_angle        | Shutter angle in degrees                                                                                                                                    |
| film/frame_id             | 32 character frame identification                                                                                                                           |
| film/slate_info           | 100 character slate info                                                                                                                                    |
| tv/time_code              | SMPTE time code as an ASCII string (e.g. 01:02:03:04)                                                                                                       |
| tv/user_bits              | SMPTE user bits as an ASCII string (e.g. 01:02:03:04)                                                                                                       |
| tv/interlace              | Interlace (0=no, 1=2:1)                                                                                                                                     |
| tv/field_num              | Field number                                                                                                                                                |
| tv/video_signal           | Video signal standard 0-254 (see DPX spec)                                                                                                                  |
| tv/horizontal_sample_rate | Horizontal sampling rate in Hz                                                                                                                              |
| tv/vertical_sample_rate   | Vertical sampling rate in Hz                                                                                                                                |
| tv/frame_rate             | Temporal sampling rate or frame rate in Hz                                                                                                                  |
| tv/time_offset            | Time offset from sync to first pixel in ms                                                                                                                  |
| tv/gamma                  | Gamma                                                                                                                                                       |
| tv/black_level            | Black level                                                                                                                                                 |
| tv/black_gain             | Black gain                                                                                                                                                  |
| tv/break_point            | Breakpoint                                                                                                                                                  |
| tv/white_level            | White level                                                                                                                                                 |
| tv/integration_times      | Integration times                                                                                                                                           |
| source/x_offset           | X offset                                                                                                                                                    |
| source/y_offset           | X offset                                                                                                                                                    |
| source/x_center           | X center                                                                                                                                                    |
| source/y_center           | Y center                                                                                                                                                    |
| source/x_original_size    | X original size                                                                                                                                             |
| source/y_original_size    | Y original size                                                                                                                                             |
| source/file_name          | Source file name                                                                                                                                            |
| source/creation_time      | Source creation time YYYY:MM:DD:hh:mm:ssTZ                                                                                                                  |
| source/input_dev          | Input device name                                                                                                                                           |
| source/input_dev          | Input device serial number                                                                                                                                  |
| source/border_XL          | Border validity left                                                                                                                                        |
| source/border_XR          | Border validity right                                                                                                                                       |
| source/border_YT          | Border validity top                                                                                                                                         |
| source/border_YB          | Border validity bottom                                                                                                                                      |
| source/pixel_aspect_H     | Pixel aspect ratio horizontal component                                                                                                                     |
| source/pixel_aspect_V     | Pixel aspect ratio vertical component                                                                                                                       |

This example set the start time code of the DPX sequence:

```
shell> rvio in.#.tif -o out.#.dpx -outparams tv/time_code=00:11:22:00
```

It is also possible to set the alignment of pixel data relative to the start of the file using alignment. For example, to force the pixel data to start at byte 4096 in the DPX file:

```
shell> rvio in.#.tif -o out.#.dpx -outparams alignment=4096
```

The smallest value for alignment is 2048 which includes the size of the default DPX headers.

The DPX writer cannot automatically pass through header fields from input DPX images to the output DPX images.

To set the project header value:

```
rvio in.#.exr -o out.#.dpx -outparams "project=THE PROJECT"
```

To set colorspace header values:

```
rvio in.#.exr -o out.#.dpx -outparams transfer=LINEAR colorimetric=REC709
```
