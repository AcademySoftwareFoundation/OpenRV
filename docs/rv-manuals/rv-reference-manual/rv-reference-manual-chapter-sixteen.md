# Chapter 16 - Node Reference

This chapter has a section for each type of node in RV's image processing graph. The properties and descriptions listed here are the default properties. Any top level node that can be seen in the session manager can have the “name” property of the “ui” component set in order to control how the node is listed.

### RVCache

The RVCache node has no external properties.

### RVCacheLUT and RVLookLUT


The RVCacheLUT is applied in software before the image is cached and before any software resolution and bit depth changes. The RVLookLUT is applied just before the display LUT but is per-source.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| lut.lut | float | div 3 | Contains either a 3D or a channel look LUT |
| lut.prelut | float | div 3 | Contains a channel pre-LUT |
| lut.inMatrix | float | 16 | Input color matrix |
| lut.outMatrix | float | 16 | Output color matrix |
| lut.scale | float | 1 | LUT output scale factor |
| lut.offset | float | 1 | LUT output offset |
| lut.file | string | 1 | Path of LUT file to read when RV session is loaded |
| lut.size | int | 1 or 3 | With 1 size value, the look LUT is a channel LUT of the specified size, if there are 3 values the look LUT is a 3D LUT with the dimensions indicated |
| lut.active | int | 1 | If non-0 the LUT is active |
| lut:output.size | int | 1 or 3 | The resampled LUT output size |
| lut:output.lut | float or half | div 3 | The resampled output LUT |
| lut:output.prelut | float or half | div 3 | The resampled output pre-LUT |

### RVCDL


This node can be used to load CDL properties from CCC, CC, and CDL files on disk.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| node.active | int | 1 | If non-0 the CDL is active. A value of 0 turns off the node. |
| node.colorspace | string | 1 | Can be "rec709", "aces", or "aceslog" and the default is "rec709". |
| node.file | string | 1 | Path of CCC, CC, or CDL file from which to read properties. |
| node.slope | float[3] | 1 | Color Decision List per-channel slope control |
| node.offset | float[3] | 1 | Color Decision List per-channel offset control |
| node.power | float[3] | 1 | Color Decision List per-channel power control |
| node.saturation | float | 1 | Color Decision List saturation control |
| node.noClamp | int | 1 | Set to 1 to remove clamping from CDL equations |

### RVChannelMap


This node can be used to remap channels that may have been labeled incorrectly.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| format.channels | string | >= 0 | An array of channel names. If the property is empty the image will pass though the node unchanged. Otherwise, only those channels appearing in the property array will be output. The channel order will be the same as the order in the property. |

### RVColor


The color node has a large number of color controls. This node is usually evaluated on the GPU, except when normalize is 1. The CDL is applied after linearization and linear color changes.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| color.normalize | int | 1 | Non-0 means to normalize the incoming pixels to [0,1] |
| color.invert | int | 1 | If non-0, invert the image color using the inversion matrix (See User's Manual) |
| color.gamma | float[3] | 1 | Apply a gamma. The default is [1.0, 1.0, 1.0]. The three values are applied to R G and B channels independently. |
| color.offset | float[3] | 1 | Color bias added to incoming color channels. Default = 0 (not bias). Each component is applied to R G B independently. |
| color.scale | float[3] | 1 | Scales each channel by the respective float value. |
| color.exposure | float[3] | 1 | Relative exposure in stops. Default = [0, 0, 0], See user's manual for more information on this. Each component is applied to R G and B independently. |
| color.contrast | float[3] | 1 | Contrast applied per channel (see User's Manual) |
| color.saturation | float | 1 | Relative saturation (see User's Manual) |
| color.hue | float | 1 | Hue rotation in radians (see User's Manual) |
| color.active | int | 1 | If 0, do not apply any color transforms. Turns off the node. |
| CDL.slope | float[3] | 1 | Color Decision List per-channel slope control |
| CDL.offset | float[3] | 1 | Color Decision List per-channel offset control |
| CDL.power | float[3] | 1 | Color Decision List per-channel power control |
| CDL.saturation | float | 1 | Color Decision List saturation control |
| CDL.noClamp | int | 1 | Set to 1 to remove clamping from CDL equations |
| luminanceLUT.lut | float | div 3 | Luminance LUT to be applied to incoming image. Contains R G B triples one after another. The LUT resolution |
| luminanceLUT.max | float | 1 | A scale on the output of the Luminance LUT |
| luminanceLUT.active | int | 1 | If non-0, luminance LUT should be applied |
| luminanceLUT:output.size | int | 1 | Output Luminance lut size |
| luminanceLUT:output.lut | float | div 3 | Output resampled luminance LUT |

### RVDispTransform2D


This node is used to do any scaling or translating of the corresponding view group.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| transform.translate | float[2] | 1 | Viewing translation |
| transform.scale | float[2] | 1 | Viewing scale |

### RVDisplayColor


This node is used by default by any display group as part of its color management pipeline.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| color.channelOrder | string | 1 | A four character string containing any of the characters [RGBA10]. The order allows permutation of the normal R G B and A channels as well as filling any channel with 1 or 0. |
| color.channelFlood | int | 1 | If 0 pass the channels through as they are. When the value is 1, 2, 3, or 4, the R G B or A channels are used to flood the R G and B channels. When the value is 5, the luminance of each pixel is computed and displayed as a gray scale image. |
| color.gamma | float | 1 | A single gamma value applied to all channels, default = 1.0 |
| color.sRGB | int | 1 | If non-0 a linear to sRGB space transform occurs |
| color.Rec709 | int | 1 | If non-0 the Rec709 transfer function is applied |
| color.brightness | float | 1 | In relative stops, the final pixel values are brightened or dimmed according to this value. Occurs after all color space transforms. |
| color.outOfRange | int | 1 | If non-0 pass pixels through an out of range filter. Channel values in the (0,1] are set to 0.5, channel values [-inf,0] are set to 0 and channel values (1,inf] are set to 1.0. |
| color.active | int | 1 | If 0 deactivate the display node |
| lut.lut | float | div 3 | Contains either a 3D or a channel display LUT |
| lut.prelut | float | div 3 | Contains a channel pre-LUT |
| lut.scale | float | 1 | LUT output scale factor |
| lut.offset | float | 1 | LUT output offset |
| lut.inMatrix | float | 16 | Input color matrix |
| lut.outMatrix | float | 16 | Output color matrix |
| lut.file | string | 1 | Path of LUT file to read when RV session is loaded |
| lut.size | int | 1 or 3 | With 1 size value, the display LUT is a channel LUT of the specified size, if there are 3 values the display LUT is a 3D LUT with the dimensions indicated |
| lut.active | int | 1 | If non-0 the display LUT is active |
| lut:output.size | int | 1 or 3 | The resampled LUT output size |
| lut:output.lut | float or half | div 3 | The resampled output LUT |
| lut:output.prelut | float or half | div 3 | The resampled output pre-LUT |

### RVDisplayGroup and RVOutputGroup


The display group provides per device display conditioning. The output group is the analogous node group for RVIO. The display groups are never saved in the session, but there is only one output group and it is saved for RVIO. There are no user external properties at this time.

### RVDisplayStereo


This node governs how to handle stereo playback including controlling the placement of stereo sources.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| rightTransform.flip | int | 1 | Flip the right eye top to bottom. |
| rightTransform.flop | int | 1 | Flop the right eye left to right. |
| rightTransform.rotate | float | 1 | Rotation of right eye in degrees. |
| rightTransform.translate | float[2] | 1 | Translation offset in X and Y for the right eye. |
| stereo.relativeOffset | float | 1 | Relative stereo offset for both eyes. |
| stereo.rightOffset | float | 1 | Stereo offset for right eye only. |
| stereo.swap | int | 1 | If set to 1 treat left eye as right and right eye as left. |
| stereo.type | string | 1 | Stereo mode in use. For example: left, right, pair, mirror, scanline, anaglyph, checker... (default is off) |

### RVFileSource


The source node controls file I/O and organize the source media into layers (in the RV sense). It has basic controls needed to mix the layers together.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| media.movie | string | > 1 | The movie, image, audio files and image sequence names. Each name is a layer in the source.There is typically at least one value in this property |
| group.fps | float | 1 | Overrides the fps found in any movie or image file or if none is found overrides the default fps of 24. |
| group.volume | float | 1 | Relative volume. This can be any positive number or 0. |
| group.audioOffset | float | 1 | Audio offset in seconds. All audio layers will be offset. |
| group.rangeOffset | int | 1 | Shifts the start and end frame numbers of all image media in the source. |
| group.rangeStart | int | 1 | Resets the start frame of all image media to given value. This is an optional property. It must be created to be set and removed to unset. |
| group.balance | float | 1 | Range of [-1,1]. A value of 0 means the audio volume is the same for both the left and right channels. |
| group.noMovieAudio | int | 1 | Do not use audio tracks in movies files |
| cut.in | int | 1 | The preferred start frame of the sequence/movie file |
| cut.out | int | 1 | The preferred end frame of the sequence/movie file |
| request.readAllChannels | int | 1 | If the value is 1 and the image format can read multiple channels, it is requested to read all channels in the current image layer and view. |
| request.imageComponent | string | 2, 3, or 4 | This array is of the form: type, view, [layer[, channel]]. The type describes what is defined in the remainder of the array. The type may be one of ”view”, ”layer”, or ”channel”. The 2nd element of the array must be defined and is the value of the view. If there are 3 elements defined then the 3rd is the layer name. If there are 4 elements defined then the 4th is the channel name. |
| request.stereoViews | string | 0 or 2 | If there are values in this property, they will be passed to the image reader when in stereo viewing mode as requested view names for the left and right eyes. |
| attributes.key | string, int, or float | 1 | This optional container of properties will get automatically included in the metadata associated with the source. The key can be any string and will be displayed as the metadata item name when displayed in the Image Info. The value of the property will be displayed as the value of the metadata. |

### RVFolderGroup


The folder group contains either a SwitchGroup or LayoutGroup which determines how it is displayed.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |
| mode.viewType | string | 1 | Either “switch” or “layout”. Determines how the folder is displayed. |

### RVFormat


This node is used to alter geometry or color depth of an image source. It is part of an RVSourceGroup.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| geometry.xfit | int | 1 | Forces the resolution to a specific width |
| geometry.yfit | int | 1 | Forces the resolution to a specific height |
| geometry.xresize | int | 1 | Forces the resolution to a specific width |
| geometry.yresize | int | 1 | Forces the resolution to a specific height |
| geometry.scale | float | 1 | Multiplier on incoming resolution. E.g., 0.5 when applied to 2048x1556 results in a 1024x768 image. |
| geometry.resampleMethod | string | 1 | Method to use when resampling. The possible values are area, cubic, and linear, |
| crop.active | int | 1 | If non-0 cropping is active |
| crop.xmin | int | 1 | Minimum X value of crop in pixel space |
| crop.ymin | int | 1 | Minimum Y value of crop in pixel space |
| crop.xmax | int | 1 | Maximum X value of crop in pixel space |
| crop.ymax | int | 1 | Maximum Y value of crop in pixel space |
| uncrop.active | int | 1 | In non-0 uncrop region is used |
| uncrop.x | int | 1 | X offset of input image into uncropped image space |
| uncrop.y | int | 1 | Y offset of input image into uncropped image space |
| uncrop.width | int | 1 | Width of uncropped image space |
| uncrop.height | int | 1 | Height of uncropped image space |
| color.maxBitDepth | int | 1 | One of 8, 16, or 32 indicating the maximum allowed bit depth (for either float or integer pixels) |
| color.allowFloatingPoint | int | 1 | If non-0 floating point images will be allowed on the GPU otherwise, the image will be converted to integer of the same bit depth (or the maximum bit depth). |
|  |  |  |  |

### RVImageSource


The RV image source is subset of what RV can handle from an external file (basically just EXR). Image sources can have multiple views each of which have multiple layers. However, all views must have the same layers. Image sources cannot have layers within layers, orphaned channels, empty views, missing views, or other weirdnesses that EXR can have.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| media.movie | string | > 1 | The movie, image, audio files and image sequence names. Each name is a layer in the source.There is typically at least one value in this property. |
| media.name | string | 1 | The name for this image. |
| cut.in | int | 1 | The preferred start frame of the sequence/movie file. |
| cut.out | int | 1 | The preferred end frame of the sequence/movie file. |
| image.channels | string | 1 | String representing the channels in the image. |
| image.layers | string | > 1 | List of strings representing the layers in the image. |
| image.views | string | > 1 | List of strings representing the views in the image. |
| image.defaultLayer | string | 1 | String representing the layer from image.layers that should be treated as default layer. |
| image.defaultView | string | 1 | String representing the view from image.views that should be treated as default view. |
| image.start | int | 1 | First frame of the source. |
| image.end | int | 1 | Last frame of the source. |
| image.inc | int | 1 | Number of frames to step by. |
| image.fps | float | 1 | Frame rate of source in float ratio of frames per second. |
| image.pixelAspect | float | 1 | Image aspect ratio as a float of width over height. |
| image.uncropHeight | int | 1 | Height of uncropped image space. |
| image.uncropWidth | int | 1 | Width of uncropped image space. |
| image.uncropX | int | 1 | X offset of image into uncropped image space. |
| image.uncropY | int | 1 | Y offset of image into uncropped image space. |
| image.width | int | 1 | Image width in integer pixels. |
| image.height | int | 1 | Image height in integer pixels. |
| request.imageChannelSelection | string | Any | Any values are considered image channel names. These are passed to the image readers with the request that only these layers be read from the image pixels. |
| request.imageComponent | string | 2, 3, or 4 | This array is of the form: type, view, [layer[, channel]]. The type describes what is defined in the remainder of the array. The type may be one of ”view”, ”layer”, or ”channel”. The 2nd element of the array must be defined and is the value of the view. If there are 3 elements defined then the 3rd is the layer name. If there are 4 elements defined then the 4th is the channel name. |
| request.stereoViews | string | 0 or 2 | If there are values in this property, they will be passed to the image reader when in stereo viewing mode as requested view names for the left and right eyes. |
| attributes.key | string, int, or float | 1 | This optional container of properties will get automatically included in the metadata associated with the source. The key can be any string and will be displayed as the metadata item name when displayed in the Image Info. The value of the property will be displayed as the value of the metadata. |

### RVLayoutGroup


The source group contains a single chain of nodes the leaf of which is an RVFileSource or RVImageSource. It has a single property.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |
| layout.mode | string | 1 | The string mode that dictates the way items are layed out. Possible values are: packed, packed2, row, column, and grid (default is packed). |
| layout.spacing | float | 1 | Scale the items in the layout. Legal values are between 0.0 and 1.0. |
| layout.gridColumns | int | 1 | When in grid mode constrain grid to this many columns. If this set to 0, then the number of columns will be determined by gridRows. If both are 0, then both will be automatically calculated. |
| layout.gridRows | int | 1 | When in grid mode constrain grid to this many rows. If this is set to 0, then the number of rows will be determined by gridColumns. This value is ignored when gridColumns is non-zero. |
| timing.retimeInputs | int | 1 | Retime all inputs to the output fps if 1 otherwise play back their frames one at a time at the output fps. |

### RVLensWarp


This node handles the pixel aspect ratio of a source group. The lens warp node can also be used to perform radial and/or tangential distortion on a frame. It implements the [Brown's distortion model](http://en.wikipedia.org/wiki/Distortion_%28optics%29) (similar to [that adopted by OpenCV](http://opencv.willowgarage.com/documentation/camera_calibration_and_3d_reconstruction.html) or Adobe Lens Camera Profile model) and 3DE4's Anamorphic Degree6 model. This node can be used to perform operations like lens distortion or artistic lens warp effects.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| warp.pixelAspectRatio | float | 1 | If non-0 set the pixel aspect ratio. Otherwise use the pixel aspect ratio reported by the incoming image. (default 0, ignored) |
| warp.model | string |  | Lens model: choices are “brown”, “opencv”, “pfbarrel”, “adobe”, “3de4_anamorphic_degree_6, “rv4.0.10”. |
| warp.k1 | float | 1 | Radial coefficient for r^2 (default 0.0)Applicable to “brown”, “opencv”, “pfbarrel”, “adobe”. |
| warp.k2 | float | 1 | Radial coefficient for r^4 (default 0.0)Applicable to “brown”, “opencv”, “pfbarrel”, “adobe”. |
| warp.k3 | float | 1 | Radial coefficient for r^6 (default 0.0)Applicable to “brown”, “opencv”, “adobe”. |
| warp.p1 | float | 1 | First tangential coefficient (default 0.0)Applicable to “brown”, “opencv”, “adobe”. |
| warp.p2 | float | 1 | Second tangential coefficient (default 0.0)Applicable to “brown”, “opencv”, “adobe”. |
| warp.cx02 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy02 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx22 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy22 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx04 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy04 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx24 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy24 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx44 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy44 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx06 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy06 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx26 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy26 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx46 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy46 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cx66 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.cy66 | float | 1 | Applicable to “3de4_anamorphic_degree_6”. (default 0.0) |
| warp.center | float[2] | 1 | Position of distortion center in normalized values [0...1] (default [0.5 0.5]. Applicable to all models. |
| warp.offset | float[2] | 1 | Offset from distortion center in normalized values [0...1.0] (default [0.0 0.0]). Applicable to all models. |
| warp.fx | float | 1 | Normalized FocalLength in X (default 1.0).Applicable to “brown”, “opencv”, “adobe”, “3de4_anamorphic_degree_6”. |
| warp.fy | float | 1 | Normalized FocalLength in Y (default 1.0).Applicable to “brown”, “opencv”, “adobe”, “3de4_anamorphic_degree_6”. |
| warp.cropRatioX | float | 1 | Crop ratio of fovX (default 1.0). Applicable to all models. |
| warp.cropRatioY | float | 1 | Crop ratio of fovY (default 1.0). Applicable to all models. |
| node.active | int | 1 | If 0, do not apply any warp/pixel aspect ratio transform. Turns off the node. (default 1) |

Example use case: Using OpenCV to determine lens distort parameters for RVLensWarp node based on GoPro footage. First capture some footage of a checkboard with your GoPro. Then you can use OpenCV camera calibration approach on this footage to solve for k1,k2,k3,p1 and p2. In OpenCV these numbers are reported back as follows. For example our 1920x1440 Hero3 Black GoPro solve returned:

```
    fx=829.122253 0.000000 cx=969.551819
    0.000000 fy=829.122253 cy=687.480774
    0.000000 0.000000 1.000000
    k1=-0.198361 k2=0.028252 p1=0.000092 p2=-0.000073 
```
The OpenCV camera calibration solve output numbers are then translated/normalized to the RVLensWarpode property values as follows:

```
    warp.model = "opencv"
    warp.k1 = k1
    warp.k2 = k2
    warp.p1 = p1
    warp.p2 = p2
    warp.center = [cx/1920 cy/1440]
    warp.fx = fx/1920
    warp.fy = fy/1920 
```
e.g. mu code:

```
    set("#RVLensWarp.warp.model", "opencv");
    set("#RVLensWarp.warp.k1", -0.198361);
    set("#RVLensWarp.warp.k2", 0.028252);
    set("#RVLensWarp.warp.p1", 0.00092);
    set("#RVLensWarp.warp.p2", -0.00073);
    setFloatProperty("#RVLensWarp.warp.offset", float[]{0.505, 0.4774}, true);
    set("#RVLensWarp.warp.fx", 0.43185);
    set("#RVLensWarp.warp.fy", 0.43185); 
```
Example use case: Using Adobe LCP (Lens Camera Profile) distort parameters for RVLensWarp node. Adobe LCP files can be located in '/Library/Application Support/Adobe/CameraRaw/LensProfiles/1.0' under OSX. Adobe LCP file parameters maps to the RVLensWarp node properties as follows:

```
    warp.model = "adobe"
    warp.k1 = stCamera:RadialDistortParam1
    warp.k2 = stCamera:RadialDistortParam2
    warp.k3 = stCamera:RadialDistortParam3
    warp.p1 = stCamera:TangentialDistortParam1
    warp.p2 = stCamera:TangentialDistortParam2
    warp.center = [stCamera:ImageXCenter stCamera:ImageYCenter]
    warp.fx = stCamera:FocalLengthX
    warp.fy = stCamera:FocalLengthY 
```

### RVLinearize


The linearize node has a large number of color controls. The CDL is applied before linearization occurs.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| color.alphaType | int | 1 | By default (0), uses the alpha type reported by the incoming image. Otherwise, 1 means the alpha is premultiplied, 0 means the incoming alpha is unpremultiplied. |
| color.YUV | int | 1 | If the value is non-0, convert the incoming pixels from YUV space to linear space. |
| color.logtype | int | 1 | The default (0), means no log to linear transform, 1 uses the cineon transform (see cineon.whiteCodeValue and cineon.blackCodeValue below), 2 means use the Viper camera log to linear transform, and 3 means use LogC log to linear transform. |
| color.sRGB2linear | int | 1 | If the value is non-0, convert the incoming pixels from sRGB space to linear space. |
| color.Rec709ToLinear | int | 1 | If the value is non-0, convert the incoming pixels using the inverse of the Rec709 transfer function. |
| color.fileGamma | float | 1 | Apply a gamma to linearize the incoming image. The default is 1.0. |
| color.active | int | 1 | If 0, do not apply any color transforms. Turns off the node. |
| color.ignoreChromaticities | int | 1 | If non-0, ignore any non-Rec 709 chromaticities reported by the incoming image. |
| CDL.slope | float[3] | 1 | Color Decision List per-channel slope control. |
| CDL.offset | float[3] | 1 | Color Decision List per-channel offset control. |
| CDL.power | float[3] | 1 | Color Decision List per-channel power control. |
| CDL.saturation | float | 1 | Color Decision List saturation control. |
| CDL.noClamp | int | 1 | Set to 1 to remove clamping from CDL equations. |
| CDL.active | int | 1 | If non-0 the CDL is active. |
| lut.lut | float | div 3 | Contains either a 3D or a channel file LUT. |
| lut.prelut | float | div 3 | Contains a channel pre-LUT. |
| lut.inMatrix | float | 16 | Input color matrix. |
| lut.outMatrix | float | 16 | Output color matrix. |
| lut.scale | float | 1 | LUT output scale factor. |
| lut.offset | float | 1 | LUT output offset. |
| lut.file | string | 1 | Path of LUT file to read when RV session is loaded. |
| lut.size | int | 1 or 3 | With 1 size value, the file LUT is a channel LUT of the specified size, if there are 3 values the file LUT is a 3D LUT with the dimensions indicated. |
| lut.active | int | 1 | If non-0 the file LUT is active. |
| lut:output.size | int | 1 or 3 | The resampled LUT output size. |
| lut:output.lut | float or half | div 3 | The resampled output LUT. |

### OCIO (OpenColorIO), OCIOFile, OCIOLook, and OCIODisplay

OpenColorIO nodes can be used in place of existing RV LUT pipelines. Properties in RVColorPipelineGroup, RVLinearizePipelineGroup, RVLookPipelineGroup, and RVDisplayPipelineGroup determine whether or not the OCIO nodes are used. All OCIO nodes have the same properties and function, but their location in the color pipeline is determined by their type. The exception is the generic OCIO node which can be created by the user and used in any context.

For more information, see [Chapter 11 - OpenColorIO](.././rv-user-manual/rv-user-manual-chapter-eleven.md)

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| ocio.lut | float | div 3 | Contains a 3D LUT, size determined by ocio.lut3DSize |
| lut.prelut | float | div 3 | Currently unused |
| ocio.active | int | 1 | Non-0 means node is active |
| ocio.lut3DSize | int | 1 | 3D LUT size of all dimensions (default is 32) |
| ocio.inSpace | string | 1 | Name of OCIO input colorspace |
| ocio_context. *name* | string | 1 | Name/Value pairs for OCIO context |

### RVOverlay

Overlay nodes can be used with any source. They can be used to draw arbitrary rectangles and text over the source but beneath any annotations. Overlay nodes can hold any number of 3 types of components: **rect** components describe a rectangle to be rendered, **text** components describe a string (or an array of strings, one per frame) to be rendered, and **window** components describe a matted region to be indicated either by coloring the region outside the window, or by outlining it. The coordiates of the corners of the window may be animated by specifying one number per frame.In the below the “ **id** ” in the component name can be any string, but must be different for each component of the same type.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| overlay.nextRectId | int | 1 | (unused) |
| overlay.nextTextId | int | 1 | (unused) |
| overlay.show | int | 1 | If 1 display any rectangles/text/window entries. If 0 do not. |
| matte.show | int | 1 | If 1 display the source specific matte, not the global |
| matte.aspect | float | 1 | Aspect ratio of the source's matte |
| matte.opacity | float | 1 | Opacity of the source's matte |
| matte.heightVisible | float | 1 | Fraction of the source height that is still visible from the matte. |
| matte.centerPoint | float[2] | 1 | The center of the matte stored as X, Y in normalized coordinates. |
| rect: *id* .color | float[4] | 1 | The color of the rectangle |
| rect: *id* .width | float | 1 | The width of the rectangle in the normalized coordinate system |
| rect: *id* .height | float | 1 | The height of the rectangle in the normalized coordinate system |
| rect: *id* .position | float[2] | 1 | Location of the rectangle in the normalized coordinate system |
| rect: *id* .active | int | 1 | If 0, rect will not be rendered |
| rect: *id* .eye | int | 1 | If absent, or set to 2, the rectangle will be rendered in both stereo eyes. If set to 0 or 1, only in the corresponding eye. |
| text: *id* .pixelScale | float[2] | 1 | X and Y scaling factors for position, IE expected source resolution, if present and non-zero, position is expected in “pixels”. |
| text: *id* .position | float[2] | 1 | Location of the text (coordinate are normalized unless pixelScale is set, in which case they are in “pixels”) |
| text: *id* .color | float[4] | 1 | The color of the text |
| text: *id* .spacing | float | 1 | The spacing of the text |
| text: *id* .size | float | 1 | The size of the text |
| text: *id* .scale | float | 1 | The scale of the text |
| text: *id* .rotation | float | 1 | (unused) |
| text: *id* .font | string | 1 | The path to the .ttf (TrueType) font to use (Default is Luxi Serif) |
| text: *id* .text | string | N | Text to be rendered, if multi-valued there should be one string per frame in the expected range. |
| text: *id* .origin | string | 1 | The origin of the text box. The position property will store the location of the origin, but the origin can be on any corner of the text box or centered in between. The valid possible values for origin are top-left, top-center, top-right, center-left, center-center, center-right, bottom-left, bottom-center, bottom-right, and the empty string (which is the default for backwards compatibility). |
| text: *id* .eye | int | 1 | If absent, or set to 2, the rectangle will be rendered in both stereo eyes. If set to 0 or 1, only in the corresponding eye. |
| text: *id* .active | int | 1 | If active is 0, the text item will not be rendered |
| text: *id* .firstFrame | int | 1 | If the “text” property is multi-valued, this property indicates the frame number corresponding to the first text value. |
| text: *id* .debug | int | 1 | (unused) |
| window: *id* .eye | int | 1 | If absent, or set to 2, the rectangle will be rendered in both stereo eyes. If set to 0 or 1, only in the corresponding eye. |
| window: *id* .antialias | int | 1 | If 1, outline/window edge drawing will be antialiased. Default 0. |
| window: *id* .windowActive | int | 1 | If windowActive is 0, the window “matting” will not be rendered |
| window: *id* .outlineActive | int | 1 | If outlineActive is 0, the window outline will not be rendered |
| window: *id* .outlineWidth | float | 1 | Assuming antialias = 1, nominal width in image-space pixels of the outline (and the degree of blurriness of the matte edge). Default 3.0 |
| window: *id* .outlineBrush | string | 1 | Assuming antialias = 1, brush used to stroke the outline (choices are "gauss" or "solid"). Default is gauss. |
| window: *id* .windowColor | float[4] | 1 | The color of the window “matting”. |
| window: *id* .outlineColor | float[4] | 1 | The color of the window outline. |
| window: *id* .imageAspect | float | 1 | The expected imageAspect of the media. If imageAspect is present and non-zero, normalized window coordinates are expected. |
| window: *id* .pixelScale | float[2] | 1 | X and Y scaling factors for window coordinates, IE expected source resolution. Used to normalize window coords in “pixels”. For pixelScale to take effect, imageAspect must be missing or 0. |
| window: *id* .firstFrame | int | 1 | If any of the window coord properties is multi-valued, this property indicates the frame number corresponding to the first coord value. |
| window: *id.windowULx* | float | N | Upper left window corner (x coord). |
| window: *id.windowULy* | float | N | Upper left window corner (y coord). |
| window: *id.windowLLx* | float | N | Lower left window corner (x coord). |
| window: *id.windowLLy* | float | N | Lower left window corner (y coord). |
| window: *id.windowURx* | float | N | Upper right window corner (x coord). |
| window: *id.windowURy* | float | N | Upper right window corner (y coord). |
| window: *id.windowLRx* | float | N | Lower right window corner (x coord). |
| window: *id.windowLRy* | float | N | Lower right window corner (y coord). |

### RVPaint


Paint nodes are used primarily to store per frame annotations. Below *id* is the value of nextID at the time the paint command property was created, *frame* is the frame on which the annotation will appear, *user* is the username of the user who created the property.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| paint.nextId | int | 1 | A counter used by the annotation mode to uniquely tag annotation pen strokes and text. |
| paint.nextAnnotationId | int | 1 | (unused) |
| paint.show | int | 1 | If 1 display any paint strokes and text entries. If 0 do not. |
| paint.exclude | string | N | (unused) |
| paint.include | string | N | (unused) |
| pen: *id* : *frame* : *user* .color | float[4] | 1 | The color of the pen stroke |
| pen: *id* : *frame* : *user* .width | float | 1 | The width of the pen stroke |
| pen: *id* : *frame* : *user* .brush | string | 1 | Brush style of “gauss” or “circle” for soft or hard lines respectively |
| pen: *id* : *frame* : *user* .points | float[2] | N | Points of the stroke in the normalized coordinate system |
| pen: *id* : *frame* : *user* .debug | int | 1 | If 1 show multicolored bounding lines around the stroke. |
| pen: *id* : *frame* : *user* .join | int | 1 | The joining style of the stroke:NoJoin = 0; BevelJoin = 1; MiterJoin = 2; RoundJoin = 3; |
| pen: *id* : *frame* : *user* .cap | int | 1 | The cap style of the stroke:NoCap = 0; SquareCap = 1; RoundCap = 2; |
| pen: *id* : *frame* : *user* .splat | int | 1 |  |
| pen: *id* : *frame* : *user* .mode | int | 1 | Drawing mode of the stroke (Default if missing is 0):RenderOverMode = 0; RenderEraseMode = 1; |
| text: *id* : *frame* : *user* .position | float[2] | 1 | Location of the text in the normalized coordinate system |
| text: *id* : *frame* : *user* .color | float[4] | 1 | The color of the text |
| text: *id* : *frame* : *user* .spacing | float | 1 | The spacing of the text |
| text: *id* : *frame* : *user* .size | float | 1 | The size of the text |
| text: *id* : *frame* : *user* .scale | float | 1 | The scale of the text |
| text: *id* : *frame* : *user* .rotation | float | 1 | (unused) |
| text: *id* : *frame* : *user* .font | string | 1 | The path to the .ttf (TrueType) font to use (Default is Luxi Serif) |
| text: *id* : *frame* : *user* .text | string | 1 | Content of the text |
| text: *id* : *frame* : *user* .origin | string | 1 | The origin of the text box. The position property will store the location of the origin, but the origin can be on any corner of the text box or centered in between. The valid possible values for origin are top-left, top-center, top-right, center-left, center-center, center-right, bottom-left, bottom-center, bottom-right, and the empty string (which is the default for backwards compatibility). |
| text: *id* : *frame* : *user* .debug | int | 1 | (unused) |

### RVPrimaryConvert


The primary convert node can be used to perform primary colorspace conversion with illuminant adaptation on a frame that has been linearized. The input and output colorspace primaries are specified in terms of input and output chromaticities for red, green, blue and white points. Illuminant adaptation is implemented using the Bradford transform where the input and output illuminant are specified in terms of their white points. Illuminant adaptation is optional. Default values are set for D65 Rec709.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| node.active | int | 1 | If non-zero node is active. (default 0) |
| illuminantAdaptation.useBradfordTransform | int | 1 | If non-zero illuminant adaptation is enabled using Bradford transform. (default 1) |
| illuminantAdaptaton.inIlluminantWhite | float | 1 | Input illuminant white point. (default [0.3127 0.3290]) |
| illuminantAdaptation.outIlluminantWhite | float | 1 | Output illuminant white point. (default [0.3127 0.3290]) |
| inChromaticities.red | float[2] | 1 | Input chromaticities red point. (default [0.6400 0.3300]) |
| inChromaticities.green | float[2] | 1 | Input chromaticities green point. (default [0.3000 0.6000]) |
| inChromaticities.blue | float[2] | 1 | Input chromaticities blue point. (default [0.1500 0.0600]) |
| inChromaticities.white | float[2] | 1 | Input chromaticities white point. (default [0.3127 0.3290]) |
| outChromaticities.red | float[2] | 1 | Output chromaticities red point. (default [0.6400 0.3300]) |
| outChromaticities.green | float[2] | 1 | Output chromaticities green point. (default [0.3000 0.6000]) |
| outChromaticities.blue | float[2] | 1 | Output chromaticities blue point. (default [0.1500 0.0600]) |
| outChromaticities.white | float[2] | 1 | Output chromaticities white point. (default [0.3127 0.3290]) |

### PipelineGroup, RVDisplayPipelineGroup, RVColorPipelineGroup, RVLinearizePipelineGroup, RVLookPipelineGroup and RVViewPipelineGroup


The PipelineGroup node and the RV specific pipeline nodes are group nodes that manages a pipeline of single input nodes. There is a single property on the node which determines the structure of the pipeline. The only difference between the various pipeline node types is the default value of the property.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| pipeline.nodes | string | 1 or more | The type names of the nodes in the managed pipeline from input to output order. |

| Node Type | Default Pipeline |
| --- | --- |
| PipelineGroup | No Default Pipeline |
| RVLinearizePipelineGroup | RVLinearize |
| RVColorPipelineGroup | RVColor |
| RVLookPipelineGroup | RVLookLUT |
| RVViewPipelineGroup | No Default Pipeline |
| RVDisplayPipelineGroup | RVDisplayColor |

### RVRetime


Retime nodes are in many of the group nodes to handle any necessary time changes to match playback between sources and views with different native frame rates. You can also use them for “artistic retiming” of two varieties.The properties in the “warp” component (see below) implement a key-framed “speed warping” variety of retiming, where the keys describe the speed (as a multiplicative factor of the target frame rate - so 1.0 implies no difference, 0.5 implies half-speed, and 2.0 implies double-speed) at a given input frame. Or you can provide an explicit map of output frames from input frames with the properties in the “explicit” component (see below). Note that the warping will still make use of what it can of the “standard” retiming properties (in particular the output fps and the visual scale), but if you use explicit retiming, none of the standard properties will have any effect. The “precedence” of the retiming types depends on the active flags: if “explicit.active” is non-zero, the other properties will have no effect., and if there is no explicit retiming, warping will be active if “warp.active” is true. Please note that neither speed warping nor explicit mapping does any retiming of the input audio.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| visual.scale | float | 1 | If extending the length scale is greater than 1.0. If decreasing the length scale is less than 1.0. |
| visual.offset | float | 1 | Number of frames to shift output. |
| audio.scale | float | 1 | If extending the length scale is greater than 1.0. If decreasing the length scale is less than 1.0. |
| audio.offset | float | 1 | Number of seconds to shift output. |
| output.fps | float | 1 | Output frame rate in frames per second. |
| warp.active | int | 1 | 1 if warping should be active. |
| warp.keyFrames | int | N | Input frame numbers at which target speed should change. |
| warp.keyRates | float | N | Target speed multipliers for each input frame number above (1.0 means no speed change). |
| explicit.active | int | 1 | 1 if an explicit mapping is provided and should be used. |
| explicit.firstOutputFrame | int | 1 | The output frame range provided by the Retime node will start with this frame. The last frame provided will be determined by the length of the array in the “inputFrames” property. |
| explicit.inputFrames | int | N | Each element in this array corresponds to an output frame, and the value of each element is the input frame number that will be used to provide the corresponding output frame. |

### RVRetimeGroup


The RetimeGroup is mostly just a holder for a Retime node. It has a single property.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |

### RVSequence


Information about how to create a working EDL can be found in the User's Manual. All of the properties in the edl component should be the same size.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| edl.frame | int | N | The global frame number which starts each cut |
| edl.source | int | N | The source input number of each cut |
| edl.in | int | N | The source relative in frame for each cut |
| edl.out | int | N | The source relative out frame for each cut |
| output.fps | float | 1 | Output FPS for the sequence. Input nodes may be retimed to this FPS. |
| output.size | int[2] | 1 | The virtual output size of the sequence. This may not match the input sizes. |
| output.interactiveSize | int | 1 | If 1 then adjust the virtual output size automatically to the window size for framing. |
| output.autoSize | int | 1 | Figure out a good size automatically from the input sizes if 1. Otherwise use output.size. |
| mode.useCutInfo | int | 1 | Use cut information on the inputs to determine EDL timing. |
| mode.autoEDL | int | 1 | If non-0, automatically concatenate new sources to the existing EDL, otherwise do not modify the EDL |

### RVSequenceGroup


The sequence group contains a chain of nodes for each of its inputs. The input chains are connected to a single RVSequence node which controls timing and switching between the inputs.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |
| timing.retimeInputs | int | 1 | Retime all inputs to the output fps if 1 otherwise play back their frames one at a time at the output fps. |

### RVSession


The session node is a great place to store centrally located information to easily access from any other node or location. Almost like a global grab bag.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| matte.aspect | float | 1 | Centralized setting for the aspect ratio of the matte used in all sources. Float ratio of width divided by height. |
| matte.centerPoint | float[2] | 1 | Centralized setting for the center of the matte used in all sources. Value stored as X, Y in normalized coordinates. |
| matte.heightVisible | float | 1 | Centralized setting for the fraction of the source height that is still visible from the matte used in all sources. |
| matte.opacity | float | 1 | Centralized setting for the opacity of the matte used in all sources. 0 == clear 1 == opaque. |
| matte.show | int | 1 | Centralized setting to turn on or off the matte used in all sources. 0 == OFF 1 == ON. |

### RVSoundTrack


Used to construct the audio waveform textures.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| audio.volume | float | 1 | Global audio volume |
| audio.balance | float | 1 | [-1,1] left/right channel balance |
| audio.offset | float | 1 | Globl audio offset in seconds |
| audio.mute | int | 1 | If non-0 audio is muted |

### RVSourceGroup


The source group contains a single chain of nodes the leaf of which is an RVFileSource or RVImageSource. It has a single property.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |

### RVSourceStereo


The source stereo nodes are used to control independent eye transformations.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| stereo.swap | int | 1 | If non-0 swap the left and right eyes |
| stereo.relativeOffset | float | 1 | Offset distance between eyes, default = 0. Both eyes are offset. |
| stereo.rightOffset | float | 1 | Offset distance between eyes, default = 0. Only right eye is offset. |
| rightTransform.flip | int | 1 | If non-0 flip the right eye |
| rightTransform.flop | int | 1 | If non-0 flop the right eye |
| rightTransform.rotate | float | 1 | Right eye rotation in degrees |
| rightTransform.translate | float[2] | 1 | independent 2D translation applied only to right eye (on top of offsets) |

### RVStack


The stack node is part of a stack group and handles control for settings like compositing each layer as well as output playback timing.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| output.fps | float | 1 | Output FPS for the stack. Input nodes may be retimed to this FPS. |
| output.size | int[2] | 1 | The virtual output size of the stack. This may not match the input sizes. |
| output.autoSize | int | 1 | Figure out a good size automatically from the input sizes if 1. Otherwise use output.size. |
| output.chosenAudioInput | string | 1 | Name of input which becomes the audio output of the stack. If the value is .all. then all inputs are mixed. If the value is .first. then the first input is used. |
| composite.type | string | 1 | The compositing operation to perform on the inputs. Valid values are: over, add, difference, -difference, and replace |
| mode.useCutInfo | int | 1 | Use cut information on the inputs to determine EDL timing. |
| mode.strictFrameRanges | int | 1 | If 1 match the timeline frames to the source frames instead of retiming to frame 1. |
| mode.alignStartFrames | int | 1 | If 1 offset all inputs so they start at same frame as the first input. |

### RVStackGroup


The stack group contains a chain of nodes for each of its inputs. The input chains are connected to a single RVStack node which controls compositing of the inputs as well as basic timing offsets.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |
| timing.retimeInputs | int | 1 | Retime all inputs to the output fps if 1 otherwise play back their frames one at a time at the output fps. |

### RVSwitch


The switch node is part of a switch group and handles control for output playback timing.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| output.fps | float | 1 | Output FPS for the switch. This is normally determined by the active input. |
| output.size | int[2] | 1 | The virtual output size of the stack. This is normally determined by the active input. |
| output.autoSize | int | 1 | Figure out a good size automatically from the input sizes if 1. Otherwise use output.size. |
| output.input | string | 1 | Name of the active input node. |
| mode.useCutInfo | int | 1 | Use cut information on the inputs to determine EDL timing. |
| mode.alignStartFrames | int | 1 | If 1 offset all inputs so they start at same frame as the first input. |

### RVSwitchGroup


The switch group changes it behavior depending on which of its inputs is “active”. It contains a single Switch node to which all of its inputs are connected.

| Name | Type | Size | Description |
| --- | --- | --- | --- |
| ui.name | string | 1 | This is a user specified name which appears in the user interface. |

### RVTransform2D


The 2D transform node controls the image transformations. This node is usually evaluated on the GPU.

| Property | Type | Size | Description |
| --- | --- | --- | --- |
| transform.flip | int | 1 | non-0 means flip the image (vertically) |
| transform.flop | int | 1 | non-0 means flop the image (horizontally) |
| transform.rotate | float | 1 | Rotate the image in degrees about its center. |
| pixel.aspectRatio | float | 1 | If non-0 set the pixel aspect ratio. Otherwise use the pixel aspect ratio reported by the incoming image. |
| transform.translate | float[2] | 1 | Translation in 2D in NDC space |
| transform.scale | float[2] | 1 | Scale in X and Y dimensions in NDC space |
| stencil.visibleBox | float | 4 | Four floats indicating the left, right, top, and bottom in NDC space of a stencil box. |

### RVViewGroup


The RVViewGroup node has no external properties.