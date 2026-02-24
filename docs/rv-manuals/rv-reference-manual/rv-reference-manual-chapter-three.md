# Chapter 3 - Writing a Custom GLSL Node

RV can use custom shaders to do GPU accelerated image processing. Shaders are authored in a superset of the GLSL language. These GLSL shaders become image processing Nodes in RV's session. Note that nodes can be either “signed” or “unsigned”. As of RV6, nodes can be loaded by any product in the RV line (RV, RVIO). The most basic workflow is as follows:

* Create a file with a custom GLSL function (the Shader) using RV's extended GLSL language.
* Create a GTO node definition file which references the Shader file.
* Test and adjust the shader/node as necessary.
* Place the node definition and shader in the RV_SUPPORT_PATH under the Nodes directory for use by other users.

### 3.1 Node Definition Files

Node definition files are GTO files which completely describe the operation of an image processing node in the image/audio processing graph.A node definition appears as a single GTO object of type IPNodeDefinition. This makes it possible for a node definition to appear in a session file directly or in an external definition file which can contain a library of definition objects.The meat of a node definition is source code for a kernel function written in an augmented version of GLSL which is described below.The following example defines a node called "Gamma" which takes a single input image and applies a gamma correction:

```
 GTOa (4)

Gamma : IPNodeDefinition (1)
{
    node
    {
        string evaluationType = "color"
        string defaultName = "gamma"
        string creator = "Tweak Software"
        string documentation = "Gamma"
        int userVisible = 1
    }

    render
    {
        int intermediate = 0
    }

    function
    {
        string name = "main" # OPTIONAL
       string glsl = "vec4 main (const in inputImage in0, const in vec3 gamma) { return vec4(pow(in0().rgb, gamma), in0().a); }"
    }

    parameters
    {
        float[3] gamma = [ [ 0.4545 0.4545 0.4545 ] ]
    }
} 
```

### 3.2 Fields in the IPNodeDefinition

node.evaluationType

one of:

| | |
| --- | --- |
| color | one input, per-pixel operations only |
| filter | one input, multiple input pixels sampled to create one output pixel |
| transition | two inputs, an animated transition |
| merge | one or more inputs, typically per-pixel operation |
| combine | one input to node, many inputs to function, pulls views, layers, eyes, multiple frames from input |

node.defaultName

the default name prefix for newly instantiated nodes

node.creator

documentation about definition author

node.documentation

Possibly html documentation string. In practice this may be quite large

node.userVisible

if non-0 a user can create this node directly otherwise only programmatically

render.intermediate

if non-0 the node results are forced to be cached

function.name

the name of the entry point in source code. By default this is main

function.fetches

approximate number of fetches performed by function. This is meaningful for filters. E.g. a 3x3 blur filter does 9 fetches.

function.glsl

Source code for the function in the augmented GLSL language. Alternately this can be a file URL pointing to the location of a separate text file containing the source code. See below for more details on file URL handling.

parameters

bindable parameters should be given default values in the parameters component. Special variables need not be given default values. (e.g. input images, the current frame, etc).

#### 3.2.1 The “combine” Evaluation Type

A “combine” node will evaluate its single input once for each parameter to the shader of type "inputImage".The names of the inputImage parameters in the shader may be chosen to be meaningful to the shader writer; they are not meaningful to the evaluation of the combine node. The order of the inputImage parameters in the shader parameter list will correspond to the multiple evaluations of the node's input (see below).Each time the input is evaluated, there are a number of variations that can be made in the context by way of properties specified in the node definition. To be clear, these properties are specified in the “parameters” section of the node definition, but they are “evaluation parameters” not shader parameters. These are:

| | |
| --- | --- |
| eye | stereo eye, int, 0 for left, 1 for right |
| channel | color channel, string, eg "R" or "Z" |
| layer | named image layer, string, typically from EXR file |
| view | named image view, string, typically from EXR file |
| frame | absolute frame number, int |
| offset | frame number offset, int |

A context-modifying property has 3 parts: the name (see above), an "inputImage index" (the int tacked onto the name), and the value. The affect of the parameter is that the context of the evaluation of the input specified by the index will be modified by the value. So for example "int eye0 = 1" means that the "eye" parameter of the context used in the first evaluation of the input will be set to "1".So for example, suppose a “StereoDifference” has this definition:

```
 StereoDifference : IPNodeDefinition (1)
{
    node
    {
        string evaluationType = "combine"
    }
    function
    {
        string glsl = "file://${HERE}/StereoQC.glsl"
    }
    parameters
    {
        int eye0 = 0
        int eye1 = 1
    }
}  
```

And this shader parameter list:

```
 vec4 main (const in inputImage left, const in inputImage right) 
```

Then:

* It's a Combine node, so it's single input can be evaluated multiple times.
* The shader has two inputImage parameters so the input will be evaluated twice.
* The node definition contains "eye" parameters, so the eye value of the evaluation context will differ in different evaluations of the input.
* In the first evaluation of the input (index = 0), the context's eye value will be set to 0, and in the second it will be set to 1.
* The results of each evaluation of the input is made available to the shader in the corresponding inputImage parameter.

As another example, here's a “FrameBlend” node:

```
 FrameBlend : IPNodeDefinition (1)
{
    node
    {
        string evaluationType = "combine"
    }
    function
    {
        string glsl = "file://${HERE}/FrameBlend.glsl"
    }
    parameters
    {
        int offset0 = -2
        int offset1 = -1
        int offset2 = 0
        int offset3 = 1
        int offset4 = 2
    }
} 
```

And this shader parameter list:

```
 vec4 main (const in inputImage in0,
           const in inputImage in1,
           const in inputImage in2,
           const in inputImage in3,
           const in inputImage in4) 
```

So the result is that the input to the FrameBlend node will be evaluated 5 times, and in each case the evaluation context will have a frame value that is equal to the incoming frame value, plus the corresponding offset. Note that the shader doesn't know anything about this, and from it's point of view it has 5 input images.

### 3.3 Alternate File URL

Language source code can be either inlined for a self contained definition or can be a modified file URL which points to an external file. An example file URL might be:

```
 file:///Users/foo/glsl/foo_shader_source.glsl 
```

If the node definition reader sees a file URL it will also perform variable substitution from the environment and any special predefined variables. For example if the $HOME environment variable exists the following would be equivalent on a Mac:

```
 file://${HOME}/glsl/foo_shader_source.glsl 
```

There is currently one special variable defined called $HERE which has the value of the directory in which the definition file lives. So if for example the node definition file lives in the filesystem like so:

```
/Users/foo/nodes/my_nodes.gto
/Users/foo/nodes/glsl/node1_source_code.glsl
/Users/foo/nodes/glsl/node2_source_code.glsl
/Users/foo/nodes/glsl/node3_source_code.glsl 
```

and it references the GLSL files mentioned above then valid file URLs for the source files would like this:

```
file://${HERE}/glsl/node1_source_code.glsl
file://${HERE}/glsl/node2_source_code.glsl
file://${HERE}/glsl/node3_source_code.glsl 
```

### 3.4 Augmented GLSL Syntax

GLSL source code can contain any set of functions and global static data but may not contain any uniform block definitions. Uniform block values are managed by the underlying renderer.

#### 3.4.1 The main() Function

The name of the function which serves as the entry point must be specified if it's not main().The main() function must always return a vec4 indicating the computed color at the current pixel.For each input to a node there should be a parameter of type inputImage. The parameters are applied in the order they appear. So the first node image input is assigned to the first inputImage parameter and so on.There are four special parameters which are supplied by the renderer:

| | |
| --- | --- |
| float frame | The current frame number (local to the node) |
| float fps | The current frame rate (local to the node) |
| float baseFrame | The current global frame number |
| float stereoEye | The current stereo eye (0=left,1=right,2=default) |

Table 3.1:Special Parameters to main() FunctionAny additional parameters are searched for in the 'parameters' component of the node. When a node is instantiated this will be populated by additional parameters to the main() function.For example, the Gamma node defined above has the following main() function:

```
vec4 main (const in inputImage in0, const in vec3 gamma)
{
    vec4 P = in0();
    return vec4(pow(P.rgb, gamma), P.a);
} 
```

In this case the node can only take a single input and will have a property called parameters.gamma of type float[3]. By changing the gamma property, the user can modify the behavior of the Gamma node.

#### 3.4.2 The inputImage Type

A new type inputImage has been added to GLSL. This type represents the input images to the node. So a node with one image argument must take a single inputImage argument. Likewise, a two input node should take two such arguments.There are a number of operations that can be done on a inputImage object. For the following examples the parameter name will be called i.

| | |
| --- | --- |
| i() | Returns the current pixel value as a vec4. Functions which only call this operator on inputImage parameters can be of type "color" |
| i(vec2 *OFF* ) | If *P* is the current pixel location this returns the pixel at *OFF* + *P* |
| i.size() | Returns a vec2 (width,height) indicating the size of the input image |
| i.st | Returns the absolute current pixel coordinates ([0,width], [0,height]) with swizzling |

Table 3.2:Type inputImage OperationsUse of the inputImage type as a function argument is limited to the main() function. Use of the inputImage type as a function should be minimized where possible e.g. the result should be stored into a local variable and the local variable used there after. For example:

```
vec4 P = i();
return vec4(P.rgb * 0.5, P.a); 
```

**NOTE** : The *st* value return by an inputImage has a value ranging from 0 to the width in X and 0 to the height in Y. So for example, the pixel value of the first pixel in the image is located at (0.5, 0.5) not at (0, 0). Similarily, the last pixel in the image is located at (width-0.5, height-0.5) not (width-1, height-1) as might be expected. See ARB_texture_rectangle for information on why this is. In GLSL 1.5 and greater the *rectangle* coordinates are built into the language.

#### 3.4.3 The outputImage Type

The type outputImage has also been added. This type provides information about the output framebuffer.The main() function may have a single outputImage parameter. You cannot pass an outputImage to auxiliary functions nor can you have outputImage parameter to an auxiliary function. You can pass the results of operations on the outputImage object to other functions.outputImage has the following operations:

| | |
| --- | --- |
| w.st | Returns the absolute fragment coordinate with swizzling |
| w.size() | Returns the size of the output framebuffer as a vec2 |

Table 3.3:Type outputImage Operations

#### 3.4.4 Use of Samplers

Samplers can be used as inputs to node functions. The sampler name and type must match an existing parameter property on the node. So for example a 1D sampler would correspond to a 1D property the value of which is a scalar array. A 3D sampler would have a type like float[3,32,32,32] if it were an RGB 32^3 LUT.

| | |
| --- | --- |
| sampler1D | *type* [ *D* , *X* ] |
| sampler2D | *type* [ *D* , *X* , *Y* ] |
| sampler2DRect | *type* [ *D* , *X* , *Y* ] |
| sampler3D | *type* [ *D* , *X* , *Y* , *Z* ] |

Table 3.4:Sampler to Parameter Type Correspondences

In the above table, *D* would normally 1, 3, or 4 for scalar, RGB, or RGBA. A value of 2 is possible but unusual.Use the new style texture() call instead of the non-overloaded pre GLSL 1.30 function calls like texture3D() or texture2DRect(). This should be the case even when the driver only supports 1.20.

### 3.5 Testing the Node Definition

Once you have a NodeDefinition GTO file that contains or references your shader code as described above, you can test the node as follows:

1. Add the node definition file to the Nodes directory on your RV_SUPPORT_PATH. For example, on Linux, you can put it in $HOME/.rv/Nodes. If the GLSL code is in a separate file, it should be in the location specified by the URL in the Node Definition file.You can use the ${HERE}/myshader.glsl notation (described above) to indicate that the GLSL is to be found in the same directory.
2. Start RV and from the Session Manager add a node with the “plus” button or the right-click menu (“New Viewable”) by choosing “Add Node by Type” and entering the type name of the new node (“Gamma” in the above example).
3. At this point you might want to save a Session File for easy testing.
4. You can now iterate by changing your shader code or the parameter values in the Session File and re-running RV to test.

### 3.6 Publishing the Node Definition

When you have tested sufficiently in RV and would like to make the new Node Definition available to other users running RV, RVIO, etc, you need to:**Make the Node Definition available to users** . RV will pick up Node Definition files from any Nodes sub-directory along the RV_SUPPORT_PATH. So your definitions can be distributed by simply inserting them into those directories, or by including them in an RV Package (any GTO/GLSL files in an RV Package will be added to the appropriate “Nodes” sub-directory when the Package is installed). With some new node types, you may want to distribute Python or or Mu code to help the user manage the creation and parameter-editing of the new nodes, so wrapping all that up in an RV Package would be appropriate in those cases.
