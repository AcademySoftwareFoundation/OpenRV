//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __Gto__EXTProtocols__h__
#define __Gto__EXTProtocols__h__
#include <Gto/Protocols.h>

#define GTO_EXT_PROTOCOL_DAMPER_CONSTRAINT "damper constraint"
#define GTO_EXT_PROTOCOL_PIN_CONSTRAINT "pin constraint"
#define GTO_EXT_PROTOCOL_RADIAL_FIELD "radial field"
#define GTO_EXT_PROTOCOL_SURFACE_FIELD "surface field"
#define GTO_EXT_PROTOCOL_WATER_GUY_FIELD "water guy"
#define GTO_EXT_PROTOCOL_AIR_FIELD "air"
#define GTO_EXT_PROTOCOL_CAMERA GTO_PROTOCOL_CAMERA
#define GTO_EXT_PROTOCOL_CHEAP_DENSITY_FIELD "cheap density"
#define GTO_EXT_PROTOCOL_CLUSTER "cluster"
#define GTO_EXT_PROTOCOL_CLUSTER_VOLUME "cluster volume"
#define GTO_EXT_PROTOCOL_COLLISION_KILL_FIELD "collision kill"
#define GTO_EXT_PROTOCOL_CONNECTION "connection"
#define GTO_EXT_PROTOCOL_DRAG_FIELD "drag"
#define GTO_EXT_PROTOCOL_EMITTER "emitter"
#define GTO_EXT_PROTOCOL_EXPR_FIELD "field expression"
#define GTO_EXT_PROTOCOL_FIELD "field"
#define GTO_EXT_PROTOCOL_GRAVITY_FIELD "gravity"
#define GTO_EXT_PROTOCOL_GRID_WIND_FIELD "grid wind"
#define GTO_EXT_PROTOCOL_HALFSPACE_FIELD "halfspace"
#define GTO_EXT_PROTOCOL_IMPORTED_DEPTH_MAP "imported depth map"
#define GTO_EXT_PROTOCOL_IMPORTED_PARTICLE "imported particle"
#define GTO_EXT_PROTOCOL_ISECT_EMITTER "isect emitter"
#define GTO_EXT_PROTOCOL_POINT_EMITTER "point emitter"
#define GTO_EXT_PROTOCOL_PRESSURE_FIELD "pressure"
#define GTO_EXT_PROTOCOL_SPHPRESSURE_FIELD "SPH pressure"
#define GTO_EXT_PROTOCOL_GOOP_FIELD "goop"
#define GTO_EXT_PROTOCOL_RIGID_BODY "rigid body"
#define GTO_EXT_PROTOCOL_SCENE "scene"
#define GTO_EXT_PROTOCOL_SPLASH_EMITTER "splash emitter"
#define GTO_EXT_PROTOCOL_SPLIT_EMITTER "split emitter"
#define GTO_EXT_PROTOCOL_DEPTH_SPLIT_EMITTER "depth split emitter"
#define GTO_EXT_PROTOCOL_SURFACE_EMITTER "surface emitter"
#define GTO_EXT_PROTOCOL_STADIUM_EMITTER "stadium emitter"
#define GTO_EXT_PROTOCOL_WIND_FIELD "wind"
#define GTO_EXT_PROTOCOL_HGT_VEL_FIELD "hgt vel field"
#define GTO_EXT_PROTOCOL_LEADING_EDGE_EMITTER "leading edge emitter"
#define GTO_EXT_PROTOCOL_SECONDARY_STRAND "secondary strand"

#define GTO_EXT_COMPONENT_BODIES "bodies"
#define GTO_EXT_COMPONENT_TETRAHEDRA "tetrahedra"
#define GTO_EXT_COMPONENT_TRIANGLES "triangles"
#define GTO_EXT_COMPONENT_FIELD "field"
#define GTO_EXT_COMPONENT_CAMERA "camera"

#define GTO_EXT_PROPERTY_FORCE "force"                           // float[3][]
#define GTO_EXT_PROPERTY_TORQUE "torque"                         // float[3][]
#define GTO_EXT_PROPERTY_ORIENTATION "orientation"               // float[4][]
#define GTO_EXT_PROPERTY_CENTER "center"                         // float[3][]
#define GTO_EXT_PROPERTY_INERTIALTENSOR "inertialTensor"         // float[9][]
#define GTO_EXT_PROPERTY_ANGULARMOMENTUM "angularMomentum"       // float[3][]
#define GTO_EXT_PROPERTY_STARTVERTEXINDEX "startVertexIndex"     // int[1][]
#define GTO_EXT_PROPERTY_STARTTETRAINDEX "startTetrahedronIndex" // int[1][]
#define GTO_EXT_PROPERTY_STARTTRIANGLEINDEX "startTriangleIndex" // int[1][]
#define GTO_EXT_PROPERTY_NUMVERTICES "numVertices"               // int[1][]
#define GTO_EXT_PROPERTY_NUMTRIANGLES "numTriangles"             // int[1][]
#define GTO_EXT_PROPERTY_NUMTETRAHEDRA "numTetrahedra"           // int[1][]
#define GTO_EXT_PROPERTY_SUBMERGED "submerged"                   // byte[1][]
#define GTO_EXT_PROPERTY_FRUSTUM "frustum"                       // float[6][]
#define GTO_EXT_PROPERTY_ASPECT "aspect"                         // float[1][]

// PROPOSED
#define GTO_EXT_PROTOCOL_COORDINATE_SPACE "coordinate space" // float[16][1]
#define GTO_EXT_COMPONENT_MATRICES "matrices"

#endif // __Gto__EXTProtocols__h__
