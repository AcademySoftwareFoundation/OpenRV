# Chapter 1 - Overview

RV comes with the source code to its user interface. The code is written in a language called Mu which is not difficult to learn if you know Python, MEL, or most other computer languages used for computer graphics. RV can use Python in a nearly interchangeable manner. If you are completely unfamiliar with programming, you may still glean information about how to customize RV in this manual; but the more complex tasks like creating a special overlay or slate for RVIO or adding a new heads-up widget to RV might be difficult to understand without help from someone more experienced.This manual does not assume you know Mu to start with, so you can dive right in. For Python, some assumptions are made. The chapters are organized with specific tasks in mind.The reference chapters contain detailed information about various internals that you can modify from the UI.Using the RV file format (.rv) is detailed in Chapter [6](rv-reference-manual-chapter-six.md#chapter-6-rv-file-format).

### 1.1 The Big Picture

RV is two different pieces of software: the core (written in C++) and the interface (written in Mu and Python). The core handles the following things:

* Image, Movie, and Audio I/O
* Caching Images and Audio
* Tracking Dependencies Among Image and Audio Operations
* Basic Image Processing in Software
* Rendering Images
* Feeding Audio to Audio Output Devices

The interface — which is available to be modified — is concerned with the following:

* Handling User Events
* Rendering Additional Information and Heads-up Widgets
* Setting and Getting State in the Image Processing Graph
* Interfacing to the Environment
* Handling User Defined Setup of Incoming Movies/Images/Audio
* High Level Features

RVIO shares almost everything with RV including the UI code (if you want it to). However it will not launch a GUI so its UI is normally non-existent. RVIO does have additional hooks for modification at the user level: overlays and leaders. Overlays are Mu scripts which allow you to render additional visual information on top of rendered images before RVIO writes them out. Leaders are scripts which generate frames from scratch (there is nothing rendered under them) and are mainly there to generate customized flexible slates automatically.

### 1.2 Drawing

In RV's user interface code or RVIO's leader and overlays it's possible draw on top of rendered frames. This is done using the industry standard API OpenGL. There are Mu modules which implement OpenGL 1.1 functions including the GLU library. In addition, there is a module which makes it easy to render true type fonts as textures (so you can scale, rotate, and composite characters as images). For Python there is PyOpenGL and related modules.Mu has a number of OpenGL friendly data types which include native support for 2D and 3D vectors and dependently typed matrices (e.g., float[4,4], float[3,3], float[4,3], etc). The Mu GL modules take the native types as input and return them from functions, but you can use normal GL documentation and man pages when programming Mu GL. In this manual, we assume you are already familiar with OpenGL. There are many resources available to learn it in a number of different programming languages. Any of those will suffice to understand it.

### 1.3 Menus

The menu bar in an RV session window is completely controlled (and created) by the UI. There are a number of ways you can add menus or override and replace the existing menu structure.Adding one or more custom menus to RV is a common customization. This manual contains examples of varying complexity to show how to do this. It is possible to create static menus (pre-defined with a known set of menu items) or dynamic menus (menus that are populated when RV is initialized based on external information, like environment variables).
