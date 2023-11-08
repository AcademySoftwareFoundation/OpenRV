# OpenColorIO Development Integration Notes

## OpenColorIO Development Integration Notes

RV has OpenColorIO (OCIO) integration.

You can use OCIO nodes in addition to or in place of RV’s "native" nodes in the Linearize, Color, Look, and Display pipelines. In each case OCIO can be used to convert from an incoming and outgoing color space with a user defined OCIO context. OCIO requires some work to set up and should be considered an advanced feature. Large facilities may find OCIO particularily useful in RV when used in conjunction with Nuke, Mari, or other products which support it.

You can learn about OpenColorIO [here](http://opencolorio.org) .

In order to use OCIO with RV a source_setup package needs to be created along with OCIO configuration files, LUTs, and an appropriate user environment. RV comes with a sample OCIO package which can be enabled via the preferences. When the OCIO source setup package is enabled, parts or all of RV’s existing color pipelines may be replaced with OCIO equivalents. Note that the sample OCIO source setup package **supplements** the default source_setup and does not replace it; there is no need to turn off the default source_setup. We highly recommend copying and customizing the sample OCIO package for real world use.

### Testing the Sample OCIO Source Setup

1.  Find the "OpenColorIO Basic Color Management" Package in the Packages tab of the Preferences. Click "Load" button next to the package and exit.
    
2.  Set the "OCIO" environment variable to the path to your favorite OCIO config file:
    
    ```
    setenv OCIO /OCIO/spi-vfx/config.ocio 
    ```
    
3.  Start RV and load an image with the color space in the name or otherwise (however is appropriate for your config).
    
    ```
    rv /media/images/ocio_special_names/marcie_clean_lg10.cin 
    ```
    
4.  Note the "OCIO" top-level menu that appears when you use RV this way. You can use this menu to chose a Linearizing transform, or Display transform, from those provided by your config.
    

### Operation of the OCIO Node

For the initial release we’re seeking feedback about how best to use OCIO in RV. To facilitate that, the current version of the OCIO node is configured to emulate three of the OCIO nuke node types: color, look, and display. There are additional OCIO nuke nodes (file transform, CDL transform, log convert) which we could also add to that list if we receive feedback indicating that’s necessary.

The OCIO nodes can be used in any of the RV pipelines as either a supplement to RV’s existing color pipeline or in place of it. The pipelines are placed in RV’s node graph in the places where all of the significant color processing occurs. By default these pipelines contain RV’s color nodes (RVLinearize, RVColor, RVLookLUT, RVDisplayColor).

The default OCIO package will swap in OCIO equivalent nodes for the existing RV nodes. For example when using the SPI OCIO config with an "lg10" cineon file the OCIO package will remove RV’s RVLinearize node in the linearization pipeline and replace it with a OCIOFile node set to take the lg10 input by default and output scene referred linear. The default OCIO package will also swap out the RVDisplayColor node (which does display color correction) for an OCIODisplay node. The OCIODisplay node is set to take scene referred linear and output to the default viewing transform (by default).

There are four OCIO node types: OCIONode, OCIOFile, OCIODisplay, and OCIOLook. All of them have the same properties; they only differ in their default configuration. Each of them can be used in any context if desired. The different node type names are primarily to make it easy to identify the nodes from the user interface.

The generic OCIONode can used as a top level (user) node. As with the RVColor node the OCIONode can therefor be used as a secondary color correction.

Note that its perfectly reasonable to use both RV and OCIO nodes in any of the pipelines. However, the example package does not do this: the intention is to show how OCIO can be used (at least for some media) *in place of* RV’s color pipeline.

Table 1. OCIO Node Properties

|  |  |
| --- | --- |
| string ocio.function | One of "color", "look", or "display" |
| float ocio.lut | Used internally to store the OCIO generated 3D LUT |
| int ocio.active | Activates/deactivates the OCIO node |
| int ocio.lut3DSize | The desired size of the OCIO generated 3D LUT (default=32) |
| string ocio.inColorSpace | The OCIO name of the input color space |
| string ocio_color.outColorSpace | The OCIO name of the output color space when ocio.function == "color" |
| string ocio_look.look | The OCIO command string for the look when ocio.function == "look" |
| int ocio_look.direction | 0=forward, 1=inverse |
| string ocio_display.display | OCIO display name when ocio.function == "display" |
| string ocio_display.view | OCIO view name when ocio.function == "display" |
| component ocio_context | String properties in this component become OCIO config name/value pairs |

### The ocio_context Component

You can add properties to the OCIO node in RV to create an OCIO context. Any string property in the component called `ocio_context` will become a name/value pair for the OCIO context.

### Feedback

* Should individual slots be hard coded to similar nuke node types? E.g. should the look OCIO slot behave only as if ocio.function == "look"? Or should we leave it up to the user to decide?
* We can add control for constructing a display pipeline like ociodisplay has (gamma, exposure, etc). Although this generally overlaps with existing RV features it may help for matching color between applications.

* * *

Last updated 2019-02-22 13:40:28 EST