# Chapter 17 - Additional GLSL Node Reference

This chapter describes the list of GLSL custom nodes that come bundled with RV. These nodes are grouped into five sections within this chapter based on the nodes "evaluationType" i.e. color, filter, transition, merge or combine. Each sub-section within a section describes a node and its parameters. For a complete description of the GLSL custom node itself, refer to the chapter on that topic i.e. "Chapter 3: Writing a Custom GLSL Node".The complete collection of GLSL custom nodes that come with each RV distribution are stored in the following two files located at:

```
 Linux & Windows:
<RV install dir>/plugins/Nodes/AdditionalNodes.gto
<RV install dir>/plugins/Support/additional_nodes/AdditionalNodes.zip

Mac:
<RV install dir>/Contents/PlugIns/Nodes/AdditionalNodes.gto
<RV install dir>/Contents/PlugIns/Support/additional_nodes/AdditionalNodes.zip  
```
The file "AdditionalNodes.gto" is a GTO formatted text file that contains the definition of all the nodes described in this chapter. All of the node definitions found in this file are signed for use by all RV4 versions. The GLSL source code that implements the node's functionality is embedded within the node definition's function block as an inlined string. In addition, the default values of the node's parameters can be found within the node definition's parameter block. The accompanying support file "AdditionalNodes.zip" is a zipped up collection of individually named node ".gto" and ".glsl" files. Users can unzip this package and refer to each node's .gto/.glsl file as examples of custom written RV GLSL nodes. Note the file "AdditionalNodes.zip" is not used by RV. Instead RV only uses "AdditionalNodes.gto" which was produced from all the files found in "AdditionalNodes.zip".These nodes can be applied through the session manager to sources, sequences, stacks, layouts or other nodes. First you select a source (for example) and from the session manager "+" pull menu select "New Node by Type" and type in the name of the node in the entry box field of the "New Node by Type" window.

### 17.1 Color Nodes


This section describes all the GLSL nodes of evaluationType "color" found in "AdditionalNodes.gto".

#### 17.1.1 Matrix3x3

This node implements a 3x3 matrix multiplication on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.m33 | float[9] | [ 1 0 0 0 1 0 0 0 1 ] |

#### 17.1.2 Matrix4x4

This node implements a 4x4 matrix multiplication on the RGBA channels of the inputImage. The inputImage alpha channel is affected by this node.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.m44 | float[16] | [ 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 ] |

#### 17.1.3 Premult

This node implements the "premultiply by alpha" operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.4 UnPremult

This node implements the "unpremultiply by alpha" (i.e. divide by alpha) operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.5 Gamma

This node implements the gamma (i.e. pixelColor^gamma) operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node. Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.gamma | float[3] | [ 0.4545 0.4545 0.4545 ] |

#### 17.1.6 CDL

This node implements the Color Description List operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Parameter lumaCoefficients defaults to full range Rec709 luma values.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.slope | float[3] | [ 1 1 1 ] |
| node.parameters.offset | float[3] | [ 0 0 0 ] |
| node.parameters.power | float[3] | [ 1 1 1 ] |
| node.parameters.saturation | float | [ 1 ] |
| node.parameters.lumaCoefficients | float[3] | [ 0.2126 0.7152 0.0722 ] |
| node.parameters.minClamp | float | [ 0 ] |
| node.parameters.maxClamp | float | [ 1 ] |

#### 17.1.7 CDLForACESLinear

This node implements the Color Description List operation in ACES linear colorspace on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Parameter lumaCoefficients defaults to full range Rec709 luma values.If the inputImage colorspace is NOT in ACES linear, but in some X linear colorspace; then one must set the 'toACES' property to the X-to-ACES colorspace conversion matrix and similarly the 'fromACES' property to the ACES-to-X colorspace conversion matrix.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.slope | float[3] | [ 1 1 1 ] |
| node.parameters.offset | float[3] | [ 0 0 0 ] |
| node.parameters.power | float[3] | [ 1 1 1 ] |
| node.parameters.saturation | float | [ 1 ] |
| node.parameters.lumaCoefficients | float[3] | [ 0.2126 0.7152 0.0722 ] |
| node.parameters.toACES | float[16] | [ 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 ] |
| node.parameters.fromACES | float[16] | [ 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 ] |
| node.parameters.minClamp | float | [ 0 ] |
| node.parameters.maxClamp | float | [ 1 ] |

#### 17.1.8 CDLForACESLog

This node implements the Color Description List operation in ACES Log colorspace on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Parameter lumaCoefficients defaults to full range Rec709 luma values.If the inputImage colorspace is NOT in ACES linear, but in some X linear colorspace; then one must set the 'toACES' property to the X-to-ACES colorspace conversion matrix and similarly the 'fromACES' property to the ACES-to-X colorspace conversion matrix.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.slope | float[3] | [ 1 1 1 ] |
| node.parameters.offset | float[3] | [ 0 0 0 ] |
| node.parameters.power | float[3] | [ 1 1 1 ] |
| node.parameters.saturation | float | [ 1 ] |
| node.parameters.lumaCoefficients | float[3] | [ 0.2126 0.7152 0.0722 ] |
| node.parameters.toACES | float[16] | [ 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 ] |
| node.parameters.fromACES | float[16] | [ 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 ] |
| node.parameters.minClamp | float | [ 0 ] |
| node.parameters.maxClamp | float | [ 1 ] |

#### 17.1.9 SRGBToLinear

This linearizing node implements the sRGB to linear transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.10 LinearToSRGB

This node implements the linear to sRGB transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.11 Rec709ToLinear

This linearizing node implements the Rec709 to linear transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.12 LinearToRec709

This node implements the linear to Rec709 transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.13 CineonLogToLinear

This linearizing node implements the Cineon Log to linear transfer function operation on the RGB channels of the inputImage. The implementation is based on Kodak specification "The Cineon Digital Film System". The inputImage alpha channel is not affected by this node.Input parameters: (values must be specified within the range [0..1023])

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.refBlack | float | 95 |
| node.parameters.refWhite | float | 685 |
| node.parameters.softClip | float | 0 |

#### 17.1.14 LinearToCineonLog

This node implements the linear to Cineon Log film transfer function operation on the RGB channels of the inputImage. The implementation is based on Kodak specification "The Cineon Digital Film System". The inputImage alpha channel is not affected by this node.Input parameters: (values must be specified within the range [0..1023])

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.refBlack | float | 95 |
| node.parameters.refWhite | float | 685 |

#### 17.1.15 ViperLogToLinear

This linearizing node implements the Viper Log to linear transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.16 LinearToViperLog

This node implements the linear to Viper Log transfer function operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.17 RGBToYCbCr601

This node implements the RGB to YCbCr 601 conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.601 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.18 RGBToYCbCr709

This node implements the RGB to YCbCr 709 conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.709 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.19 RGBToYCgCo

This node implements the RGB to YCgCo conversion operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.20 YCbCr601ToRGB

This node implements the YCbCr 601 to RGB conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.601 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.21 YCbCr709ToRGB

This node implements the YCbCr 709 to RGB conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.709 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.22 YCgCoToRGB

This node implements the YCgCo to RGB conversion operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.23 YCbCr601FRToRGB

This node implements the YCbCr 601 "Full Range" to RGB conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.601 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.24 RGBToYCbCr601FR

This node implements the RGB to YCbCr 601 "Full Range" conversion operation on the RGB channels of the inputImage. Implementation is based on ITU-R BT.601 specification. The inputImage alpha channel is not affected by this node.Input parameters: None

#### 17.1.27 Saturation

This node implements the saturation operation on the RGB channels of the inputImage. The inputImage alpha channel is not affected by this node.Parameter lumaCoefficients defaults to full range Rec709 luma values.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.saturation | float | [ 1 ] |
| node.parameters.lumaCoefficients | float[3] | [ 0.2126 0.7152 0.0722 ] |
| node.parameters.minClamp | float | [ 0 ] |
| node.parameters.maxClamp | float | [ 1 ] |

### 17.2 Transition Nodes


This section describes all the GLSL nodes of evaluationType "transition" found in "AdditionalNodes.gto".

### 17.2.1 CrossDissolve

This node implements a simple cross dissolve transition effect on the RGBA channels of two inputImage sources beginning from startFrame until (startFrame + numFrames -1). The inputImage alpha channel is affected by this node.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.startFrame | float | 40 |
| node.parameters.numFrame | float | 20 |

### 17.2.2 Wipe

This node implements a simple wipe transition effect on the RGBA channels of two inputImage sources beginning from startFrame until (startFrame + numFrames -1). The inputImage alpha channel is affected by this node.Input parameters:

| Property | Type | Default |
| --- | --- | --- |
| node.parameters.startFrame | float | 40 |
| node.parameters.numFrame | float | 20 |