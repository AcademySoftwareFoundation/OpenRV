//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <limits>

#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/FunctionObject.h>
#include <Mu/Module.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Signature.h>
#include <Mu/Thread.h>
#include <Mu/SymbolicConstant.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

#include <math.h>
#ifndef isnan
#define isnan(x) ((x) != (x))
#endif

#ifdef TWK_USE_GLEW
#include <GL/glew.h>
#endif

// AJG - CLASSIC
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
#include <windows.h>
#endif

#if defined(PLATFORM_DARWIN)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>
#elif defined(PLATFORM_LINUX)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#elif defined(PLATFORM_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <MuGL/GLModule.h>

namespace Mu
{

    using namespace std;

    void GLModule::init(const char* name, Context* context)
    {
        static bool initialized = false;

        if (!initialized)
        {
            initialized = true;
            Module* m = new GLModule(context, name);
            context->globalScope()->addSymbol(m);
        }
    }

    GLModule::GLModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    GLModule::~GLModule() {}

    void GLModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* c = (MuLangContext*)globalModule()->context();

        //
        // Instantate types used later...
        //
        c->arrayType(c->stringType(), 1, 0);   // string[]
        c->arrayType(c->boolType(), 1, 0);     // bool[]
        c->arrayType(c->intType(), 1, 0);      // int[]
        c->arrayType(c->floatType(), 1, 0);    // float[]
        c->arrayType(c->byteType(), 1, 0);     // byte[]
        c->arrayType(c->vec2fType(), 1, 0);    // vec2f[]
        c->arrayType(c->vec3fType(), 1, 0);    // vec3f[]
        c->arrayType(c->vec4fType(), 1, 0);    // vec4f[]
        c->arrayType(c->floatType(), 2, 4, 4); // float[4,4]

        c->arrayType(c->byteType(), 2, 32, 32);

#define SYMCONST(name) new SymbolicConstant(c, #name, "int", Value(int(name)))

        addSymbols(
            //
            // Note: These are ALL of the OpenGL 1.1 constants!
            //
            /* AttribMask */
            SYMCONST(GL_CURRENT_BIT), SYMCONST(GL_POINT_BIT),
            SYMCONST(GL_LINE_BIT), SYMCONST(GL_POLYGON_BIT),
            SYMCONST(GL_POLYGON_STIPPLE_BIT), SYMCONST(GL_PIXEL_MODE_BIT),
            SYMCONST(GL_LIGHTING_BIT), SYMCONST(GL_FOG_BIT),
            SYMCONST(GL_DEPTH_BUFFER_BIT), SYMCONST(GL_ACCUM_BUFFER_BIT),
            SYMCONST(GL_STENCIL_BUFFER_BIT), SYMCONST(GL_VIEWPORT_BIT),
            SYMCONST(GL_TRANSFORM_BIT), SYMCONST(GL_ENABLE_BIT),
            SYMCONST(GL_COLOR_BUFFER_BIT), SYMCONST(GL_HINT_BIT),
            SYMCONST(GL_EVAL_BIT), SYMCONST(GL_LIST_BIT),
            SYMCONST(GL_TEXTURE_BIT), SYMCONST(GL_SCISSOR_BIT),
            SYMCONST(GL_ALL_ATTRIB_BITS),

            /* ClientAttribMask */
            SYMCONST(GL_CLIENT_PIXEL_STORE_BIT),
            SYMCONST(GL_CLIENT_VERTEX_ARRAY_BIT),
            SYMCONST(GL_CLIENT_ALL_ATTRIB_BITS),

            /* BeginMode */
            SYMCONST(GL_POINTS), SYMCONST(GL_LINES), SYMCONST(GL_LINE_LOOP),
            SYMCONST(GL_LINE_STRIP), SYMCONST(GL_TRIANGLES),
            SYMCONST(GL_TRIANGLE_STRIP), SYMCONST(GL_TRIANGLE_FAN),
            SYMCONST(GL_QUADS), SYMCONST(GL_QUAD_STRIP), SYMCONST(GL_POLYGON),

            /* AccumOp */
            SYMCONST(GL_ACCUM), SYMCONST(GL_LOAD), SYMCONST(GL_RETURN),
            SYMCONST(GL_MULT), SYMCONST(GL_ADD),

            /* AlphaFunction */
            SYMCONST(GL_NEVER), SYMCONST(GL_LESS), SYMCONST(GL_EQUAL),
            SYMCONST(GL_LEQUAL), SYMCONST(GL_GREATER), SYMCONST(GL_NOTEQUAL),
            SYMCONST(GL_GEQUAL), SYMCONST(GL_ALWAYS),

            /* BlendingFactorDest */
            SYMCONST(GL_ZERO), SYMCONST(GL_ONE), SYMCONST(GL_SRC_COLOR),
            SYMCONST(GL_ONE_MINUS_SRC_COLOR), SYMCONST(GL_SRC_ALPHA),
            SYMCONST(GL_ONE_MINUS_SRC_ALPHA), SYMCONST(GL_DST_ALPHA),
            SYMCONST(GL_ONE_MINUS_DST_ALPHA),

            /* BlendingFactorSrc */
            SYMCONST(GL_DST_COLOR), SYMCONST(GL_ONE_MINUS_DST_COLOR),
            SYMCONST(GL_SRC_ALPHA_SATURATE),

            /* DrawBufferMode */
            SYMCONST(GL_NONE), SYMCONST(GL_FRONT_LEFT),
            SYMCONST(GL_FRONT_RIGHT), SYMCONST(GL_BACK_LEFT),
            SYMCONST(GL_BACK_RIGHT), SYMCONST(GL_FRONT), SYMCONST(GL_BACK),
            SYMCONST(GL_LEFT), SYMCONST(GL_RIGHT), SYMCONST(GL_FRONT_AND_BACK),
            SYMCONST(GL_AUX0), SYMCONST(GL_AUX1), SYMCONST(GL_AUX2),
            SYMCONST(GL_AUX3),

            /* ErrorCode */
            SYMCONST(GL_NO_ERROR), SYMCONST(GL_INVALID_ENUM),
            SYMCONST(GL_INVALID_VALUE), SYMCONST(GL_INVALID_OPERATION),
            SYMCONST(GL_STACK_OVERFLOW), SYMCONST(GL_STACK_UNDERFLOW),
            SYMCONST(GL_OUT_OF_MEMORY), SYMCONST(GL_TABLE_TOO_LARGE),

            /* FeedbackType */
            SYMCONST(GL_2D), SYMCONST(GL_3D), SYMCONST(GL_3D_COLOR),
            SYMCONST(GL_3D_COLOR_TEXTURE), SYMCONST(GL_4D_COLOR_TEXTURE),

            /* FeedBackToken */
            SYMCONST(GL_PASS_THROUGH_TOKEN), SYMCONST(GL_POINT_TOKEN),
            SYMCONST(GL_LINE_TOKEN), SYMCONST(GL_POLYGON_TOKEN),
            SYMCONST(GL_BITMAP_TOKEN), SYMCONST(GL_DRAW_PIXEL_TOKEN),
            SYMCONST(GL_COPY_PIXEL_TOKEN), SYMCONST(GL_LINE_RESET_TOKEN),

            /* FogMode */
            SYMCONST(GL_EXP), SYMCONST(GL_EXP2),

            /* FrontFaceDirection */
            SYMCONST(GL_CW), SYMCONST(GL_CCW),

            /* GetMapQuery */
            SYMCONST(GL_COEFF), SYMCONST(GL_ORDER), SYMCONST(GL_DOMAIN),

            /* GetPixelMap */
            SYMCONST(GL_PIXEL_MAP_I_TO_I), SYMCONST(GL_PIXEL_MAP_S_TO_S),
            SYMCONST(GL_PIXEL_MAP_I_TO_R), SYMCONST(GL_PIXEL_MAP_I_TO_G),
            SYMCONST(GL_PIXEL_MAP_I_TO_B), SYMCONST(GL_PIXEL_MAP_I_TO_A),
            SYMCONST(GL_PIXEL_MAP_R_TO_R), SYMCONST(GL_PIXEL_MAP_G_TO_G),
            SYMCONST(GL_PIXEL_MAP_B_TO_B), SYMCONST(GL_PIXEL_MAP_A_TO_A),

            /* GetPointervPName */
            SYMCONST(GL_VERTEX_ARRAY_POINTER),
            SYMCONST(GL_NORMAL_ARRAY_POINTER), SYMCONST(GL_COLOR_ARRAY_POINTER),
            SYMCONST(GL_INDEX_ARRAY_POINTER),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_POINTER),
            SYMCONST(GL_EDGE_FLAG_ARRAY_POINTER), EndArguments);

        addSymbols(
            /* GetPName */
            SYMCONST(GL_CURRENT_COLOR), SYMCONST(GL_CURRENT_INDEX),
            SYMCONST(GL_CURRENT_NORMAL), SYMCONST(GL_CURRENT_TEXTURE_COORDS),
            SYMCONST(GL_CURRENT_RASTER_COLOR),
            SYMCONST(GL_CURRENT_RASTER_INDEX),
            SYMCONST(GL_CURRENT_RASTER_TEXTURE_COORDS),
            SYMCONST(GL_CURRENT_RASTER_POSITION),
            SYMCONST(GL_CURRENT_RASTER_POSITION_VALID),
            SYMCONST(GL_CURRENT_RASTER_DISTANCE), SYMCONST(GL_POINT_SMOOTH),
            SYMCONST(GL_POINT_SIZE), SYMCONST(GL_SMOOTH_POINT_SIZE_RANGE),
            SYMCONST(GL_SMOOTH_POINT_SIZE_GRANULARITY),
            SYMCONST(GL_POINT_SIZE_RANGE), SYMCONST(GL_POINT_SIZE_GRANULARITY),
            SYMCONST(GL_LINE_SMOOTH), SYMCONST(GL_LINE_WIDTH),
            SYMCONST(GL_SMOOTH_LINE_WIDTH_RANGE),
            SYMCONST(GL_SMOOTH_LINE_WIDTH_GRANULARITY),
            SYMCONST(GL_LINE_WIDTH_RANGE), SYMCONST(GL_LINE_WIDTH_GRANULARITY),
            SYMCONST(GL_LINE_STIPPLE), SYMCONST(GL_LINE_STIPPLE_PATTERN),
            SYMCONST(GL_LINE_STIPPLE_REPEAT), SYMCONST(GL_LIST_MODE),
            SYMCONST(GL_MAX_LIST_NESTING), SYMCONST(GL_LIST_BASE),
            SYMCONST(GL_LIST_INDEX), SYMCONST(GL_POLYGON_MODE),
            SYMCONST(GL_POLYGON_SMOOTH), SYMCONST(GL_POLYGON_STIPPLE),
            SYMCONST(GL_EDGE_FLAG), SYMCONST(GL_CULL_FACE),
            SYMCONST(GL_CULL_FACE_MODE), SYMCONST(GL_FRONT_FACE),
            SYMCONST(GL_LIGHTING), SYMCONST(GL_LIGHT_MODEL_LOCAL_VIEWER),
            SYMCONST(GL_LIGHT_MODEL_TWO_SIDE), SYMCONST(GL_LIGHT_MODEL_AMBIENT),
            SYMCONST(GL_SHADE_MODEL), SYMCONST(GL_COLOR_MATERIAL_FACE),
            SYMCONST(GL_COLOR_MATERIAL_PARAMETER), SYMCONST(GL_COLOR_MATERIAL),
            SYMCONST(GL_FOG), SYMCONST(GL_FOG_INDEX), SYMCONST(GL_FOG_DENSITY),
            SYMCONST(GL_FOG_START), SYMCONST(GL_FOG_END), SYMCONST(GL_FOG_MODE),
            SYMCONST(GL_FOG_COLOR), SYMCONST(GL_DEPTH_RANGE),
            SYMCONST(GL_DEPTH_TEST), SYMCONST(GL_DEPTH_WRITEMASK),
            SYMCONST(GL_DEPTH_CLEAR_VALUE), SYMCONST(GL_DEPTH_FUNC),
            SYMCONST(GL_ACCUM_CLEAR_VALUE), SYMCONST(GL_STENCIL_TEST),
            SYMCONST(GL_STENCIL_CLEAR_VALUE), SYMCONST(GL_STENCIL_FUNC),
            SYMCONST(GL_STENCIL_VALUE_MASK), SYMCONST(GL_STENCIL_FAIL),
            SYMCONST(GL_STENCIL_PASS_DEPTH_FAIL),
            SYMCONST(GL_STENCIL_PASS_DEPTH_PASS), SYMCONST(GL_STENCIL_REF),
            SYMCONST(GL_STENCIL_WRITEMASK), SYMCONST(GL_MATRIX_MODE),
            SYMCONST(GL_NORMALIZE), SYMCONST(GL_VIEWPORT),
            SYMCONST(GL_MODELVIEW_STACK_DEPTH),
            SYMCONST(GL_PROJECTION_STACK_DEPTH),
            SYMCONST(GL_TEXTURE_STACK_DEPTH), SYMCONST(GL_MODELVIEW_MATRIX),
            SYMCONST(GL_PROJECTION_MATRIX), SYMCONST(GL_TEXTURE_MATRIX),
            SYMCONST(GL_ATTRIB_STACK_DEPTH),
            SYMCONST(GL_CLIENT_ATTRIB_STACK_DEPTH), SYMCONST(GL_ALPHA_TEST),
            SYMCONST(GL_ALPHA_TEST_FUNC), SYMCONST(GL_ALPHA_TEST_REF),
            SYMCONST(GL_DITHER), SYMCONST(GL_BLEND_DST), SYMCONST(GL_BLEND_SRC),
            SYMCONST(GL_BLEND), SYMCONST(GL_LOGIC_OP_MODE),
            SYMCONST(GL_INDEX_LOGIC_OP), SYMCONST(GL_LOGIC_OP),
            SYMCONST(GL_COLOR_LOGIC_OP), SYMCONST(GL_AUX_BUFFERS),
            SYMCONST(GL_DRAW_BUFFER), SYMCONST(GL_READ_BUFFER),
            SYMCONST(GL_SCISSOR_BOX), SYMCONST(GL_SCISSOR_TEST),
            SYMCONST(GL_INDEX_CLEAR_VALUE), SYMCONST(GL_INDEX_WRITEMASK),
            SYMCONST(GL_COLOR_CLEAR_VALUE), SYMCONST(GL_COLOR_WRITEMASK),
            SYMCONST(GL_INDEX_MODE), SYMCONST(GL_RGBA_MODE),
            SYMCONST(GL_DOUBLEBUFFER), SYMCONST(GL_STEREO),
            SYMCONST(GL_RENDER_MODE), SYMCONST(GL_PERSPECTIVE_CORRECTION_HINT),
            SYMCONST(GL_POINT_SMOOTH_HINT), SYMCONST(GL_LINE_SMOOTH_HINT),
            SYMCONST(GL_POLYGON_SMOOTH_HINT), SYMCONST(GL_FOG_HINT),
            SYMCONST(GL_TEXTURE_GEN_S), SYMCONST(GL_TEXTURE_GEN_T),
            SYMCONST(GL_TEXTURE_GEN_R), SYMCONST(GL_TEXTURE_GEN_Q),
            SYMCONST(GL_PIXEL_MAP_I_TO_I_SIZE),
            SYMCONST(GL_PIXEL_MAP_S_TO_S_SIZE),
            SYMCONST(GL_PIXEL_MAP_I_TO_R_SIZE),
            SYMCONST(GL_PIXEL_MAP_I_TO_G_SIZE),
            SYMCONST(GL_PIXEL_MAP_I_TO_B_SIZE),
            SYMCONST(GL_PIXEL_MAP_I_TO_A_SIZE),
            SYMCONST(GL_PIXEL_MAP_R_TO_R_SIZE),
            SYMCONST(GL_PIXEL_MAP_G_TO_G_SIZE),
            SYMCONST(GL_PIXEL_MAP_B_TO_B_SIZE),
            SYMCONST(GL_PIXEL_MAP_A_TO_A_SIZE), SYMCONST(GL_UNPACK_SWAP_BYTES),
            SYMCONST(GL_UNPACK_LSB_FIRST), SYMCONST(GL_UNPACK_ROW_LENGTH),
            SYMCONST(GL_UNPACK_SKIP_ROWS), SYMCONST(GL_UNPACK_SKIP_PIXELS),
            SYMCONST(GL_UNPACK_ALIGNMENT), SYMCONST(GL_PACK_SWAP_BYTES),
            SYMCONST(GL_PACK_LSB_FIRST), SYMCONST(GL_PACK_ROW_LENGTH),
            SYMCONST(GL_PACK_SKIP_ROWS), SYMCONST(GL_PACK_SKIP_PIXELS),
            SYMCONST(GL_PACK_ALIGNMENT), SYMCONST(GL_MAP_COLOR),
            SYMCONST(GL_MAP_STENCIL), SYMCONST(GL_INDEX_SHIFT),
            SYMCONST(GL_INDEX_OFFSET), SYMCONST(GL_RED_SCALE),
            SYMCONST(GL_RED_BIAS), SYMCONST(GL_ZOOM_X), SYMCONST(GL_ZOOM_Y),
            SYMCONST(GL_GREEN_SCALE), SYMCONST(GL_GREEN_BIAS),
            SYMCONST(GL_BLUE_SCALE), SYMCONST(GL_BLUE_BIAS),
            SYMCONST(GL_ALPHA_SCALE), SYMCONST(GL_ALPHA_BIAS),
            SYMCONST(GL_DEPTH_SCALE), SYMCONST(GL_DEPTH_BIAS),
            SYMCONST(GL_MAX_EVAL_ORDER), SYMCONST(GL_MAX_LIGHTS),
            SYMCONST(GL_MAX_CLIP_PLANES), SYMCONST(GL_MAX_TEXTURE_SIZE),
            SYMCONST(GL_MAX_PIXEL_MAP_TABLE),
            SYMCONST(GL_MAX_ATTRIB_STACK_DEPTH),
            SYMCONST(GL_MAX_MODELVIEW_STACK_DEPTH),
            SYMCONST(GL_MAX_NAME_STACK_DEPTH),
            SYMCONST(GL_MAX_PROJECTION_STACK_DEPTH),
            SYMCONST(GL_MAX_TEXTURE_STACK_DEPTH),
            SYMCONST(GL_MAX_VIEWPORT_DIMS),
            SYMCONST(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH),
            SYMCONST(GL_SUBPIXEL_BITS), SYMCONST(GL_INDEX_BITS),
            SYMCONST(GL_RED_BITS), SYMCONST(GL_GREEN_BITS),
            SYMCONST(GL_BLUE_BITS), SYMCONST(GL_ALPHA_BITS),
            SYMCONST(GL_DEPTH_BITS), SYMCONST(GL_STENCIL_BITS),
            SYMCONST(GL_ACCUM_RED_BITS), SYMCONST(GL_ACCUM_GREEN_BITS),
            SYMCONST(GL_ACCUM_BLUE_BITS), SYMCONST(GL_ACCUM_ALPHA_BITS),
            SYMCONST(GL_NAME_STACK_DEPTH), SYMCONST(GL_AUTO_NORMAL),
            SYMCONST(GL_MAP1_COLOR_4), SYMCONST(GL_MAP1_INDEX),
            SYMCONST(GL_MAP1_NORMAL), SYMCONST(GL_MAP1_TEXTURE_COORD_1),
            SYMCONST(GL_MAP1_TEXTURE_COORD_2),
            SYMCONST(GL_MAP1_TEXTURE_COORD_3),
            SYMCONST(GL_MAP1_TEXTURE_COORD_4), SYMCONST(GL_MAP1_VERTEX_3),
            SYMCONST(GL_MAP1_VERTEX_4), SYMCONST(GL_MAP2_COLOR_4),
            SYMCONST(GL_MAP2_INDEX), SYMCONST(GL_MAP2_NORMAL),
            SYMCONST(GL_MAP2_TEXTURE_COORD_1),
            SYMCONST(GL_MAP2_TEXTURE_COORD_2),
            SYMCONST(GL_MAP2_TEXTURE_COORD_3),
            SYMCONST(GL_MAP2_TEXTURE_COORD_4), SYMCONST(GL_MAP2_VERTEX_3),
            SYMCONST(GL_MAP2_VERTEX_4), SYMCONST(GL_MAP1_GRID_DOMAIN),
            SYMCONST(GL_MAP1_GRID_SEGMENTS), SYMCONST(GL_MAP2_GRID_DOMAIN),
            SYMCONST(GL_MAP2_GRID_SEGMENTS), SYMCONST(GL_TEXTURE_1D),
            SYMCONST(GL_TEXTURE_2D), SYMCONST(GL_TEXTURE_RECTANGLE_ARB),
            SYMCONST(GL_FEEDBACK_BUFFER_POINTER),
            SYMCONST(GL_FEEDBACK_BUFFER_SIZE),
            SYMCONST(GL_FEEDBACK_BUFFER_TYPE),
            SYMCONST(GL_SELECTION_BUFFER_POINTER),
            SYMCONST(GL_SELECTION_BUFFER_SIZE),
            SYMCONST(GL_POLYGON_OFFSET_UNITS),
            SYMCONST(GL_POLYGON_OFFSET_POINT), SYMCONST(GL_POLYGON_OFFSET_LINE),
            SYMCONST(GL_POLYGON_OFFSET_FILL),
            SYMCONST(GL_POLYGON_OFFSET_FACTOR), SYMCONST(GL_TEXTURE_BINDING_1D),
            SYMCONST(GL_TEXTURE_BINDING_2D), SYMCONST(GL_TEXTURE_BINDING_3D),
            SYMCONST(GL_VERTEX_ARRAY), SYMCONST(GL_NORMAL_ARRAY),
            SYMCONST(GL_COLOR_ARRAY), SYMCONST(GL_INDEX_ARRAY),
            SYMCONST(GL_TEXTURE_COORD_ARRAY), SYMCONST(GL_EDGE_FLAG_ARRAY),
            SYMCONST(GL_VERTEX_ARRAY_SIZE), SYMCONST(GL_VERTEX_ARRAY_TYPE),
            SYMCONST(GL_VERTEX_ARRAY_STRIDE), SYMCONST(GL_NORMAL_ARRAY_TYPE),
            SYMCONST(GL_NORMAL_ARRAY_STRIDE), SYMCONST(GL_COLOR_ARRAY_SIZE),
            SYMCONST(GL_COLOR_ARRAY_TYPE), SYMCONST(GL_COLOR_ARRAY_STRIDE),
            SYMCONST(GL_INDEX_ARRAY_TYPE), SYMCONST(GL_INDEX_ARRAY_STRIDE),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_SIZE),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_TYPE),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_STRIDE),
            SYMCONST(GL_EDGE_FLAG_ARRAY_STRIDE),

            /* GetTextureParameter */
            SYMCONST(GL_TEXTURE_WIDTH), SYMCONST(GL_TEXTURE_HEIGHT),
            SYMCONST(GL_TEXTURE_INTERNAL_FORMAT),
            SYMCONST(GL_TEXTURE_COMPONENTS), SYMCONST(GL_TEXTURE_BORDER_COLOR),
            SYMCONST(GL_TEXTURE_BORDER), SYMCONST(GL_TEXTURE_RED_SIZE),
            SYMCONST(GL_TEXTURE_GREEN_SIZE), SYMCONST(GL_TEXTURE_BLUE_SIZE),
            SYMCONST(GL_TEXTURE_ALPHA_SIZE),
            SYMCONST(GL_TEXTURE_LUMINANCE_SIZE),
            SYMCONST(GL_TEXTURE_INTENSITY_SIZE), SYMCONST(GL_TEXTURE_PRIORITY),
            SYMCONST(GL_TEXTURE_RESIDENT), EndArguments);

        addSymbols(
            /* HintMode */
            SYMCONST(GL_DONT_CARE), SYMCONST(GL_FASTEST), SYMCONST(GL_NICEST),

            /* LightParameter */
            SYMCONST(GL_AMBIENT), SYMCONST(GL_DIFFUSE), SYMCONST(GL_SPECULAR),
            SYMCONST(GL_POSITION), SYMCONST(GL_SPOT_DIRECTION),
            SYMCONST(GL_SPOT_EXPONENT), SYMCONST(GL_SPOT_CUTOFF),
            SYMCONST(GL_CONSTANT_ATTENUATION), SYMCONST(GL_LINEAR_ATTENUATION),
            SYMCONST(GL_QUADRATIC_ATTENUATION),

            /* ListMode */
            SYMCONST(GL_COMPILE), SYMCONST(GL_COMPILE_AND_EXECUTE),

            /* DataType */
            SYMCONST(GL_BYTE), SYMCONST(GL_UNSIGNED_BYTE), SYMCONST(GL_SHORT),
            SYMCONST(GL_UNSIGNED_SHORT), SYMCONST(GL_INT),
            SYMCONST(GL_UNSIGNED_INT), SYMCONST(GL_FLOAT), SYMCONST(GL_2_BYTES),
            SYMCONST(GL_3_BYTES), SYMCONST(GL_4_BYTES), SYMCONST(GL_DOUBLE),
            // SYMCONST( GL_DOUBLE_EXT ),

            /* LogicOp */
            SYMCONST(GL_CLEAR), SYMCONST(GL_AND), SYMCONST(GL_AND_REVERSE),
            SYMCONST(GL_COPY), SYMCONST(GL_AND_INVERTED), SYMCONST(GL_NOOP),
            SYMCONST(GL_XOR), SYMCONST(GL_OR), SYMCONST(GL_NOR),
            SYMCONST(GL_EQUIV), SYMCONST(GL_INVERT), SYMCONST(GL_OR_REVERSE),
            SYMCONST(GL_COPY_INVERTED), SYMCONST(GL_OR_INVERTED),
            SYMCONST(GL_NAND), SYMCONST(GL_SET),

            /* MaterialParameter */
            SYMCONST(GL_EMISSION), SYMCONST(GL_SHININESS),
            SYMCONST(GL_AMBIENT_AND_DIFFUSE), SYMCONST(GL_COLOR_INDEXES),

            /* MatrixMode */
            SYMCONST(GL_MODELVIEW), SYMCONST(GL_PROJECTION),
            SYMCONST(GL_TEXTURE),

            /* PixelCopyType */
            SYMCONST(GL_COLOR), SYMCONST(GL_DEPTH), SYMCONST(GL_STENCIL),

            /* PixelFormat */
            SYMCONST(GL_COLOR_INDEX), SYMCONST(GL_STENCIL_INDEX),
            SYMCONST(GL_DEPTH_COMPONENT), SYMCONST(GL_RED), SYMCONST(GL_GREEN),
            SYMCONST(GL_BLUE), SYMCONST(GL_ALPHA), SYMCONST(GL_RGB),
            SYMCONST(GL_RGBA), SYMCONST(GL_LUMINANCE),
            SYMCONST(GL_LUMINANCE_ALPHA),

            /* PixelType */
            SYMCONST(GL_BITMAP),

            /* PolygonMode */
            SYMCONST(GL_POINT), SYMCONST(GL_LINE), SYMCONST(GL_FILL),

            /* RenderingMode */
            SYMCONST(GL_RENDER), SYMCONST(GL_FEEDBACK), SYMCONST(GL_SELECT),

            /* ShadingModel */
            SYMCONST(GL_FLAT), SYMCONST(GL_SMOOTH),

            /* StencilOp */
            SYMCONST(GL_KEEP), SYMCONST(GL_REPLACE), SYMCONST(GL_INCR),
            SYMCONST(GL_DECR),

            /* StringName */
            SYMCONST(GL_VENDOR), SYMCONST(GL_RENDERER), SYMCONST(GL_VERSION),
            SYMCONST(GL_EXTENSIONS),

            /* TextureCoordName */
            SYMCONST(GL_S), SYMCONST(GL_T), SYMCONST(GL_R), SYMCONST(GL_Q),

            /* TextureEnvMode */
            SYMCONST(GL_MODULATE), SYMCONST(GL_DECAL),

            /* TextureEnvParameter */
            SYMCONST(GL_TEXTURE_ENV_MODE), SYMCONST(GL_TEXTURE_ENV_COLOR),

            /* TextureEnvTarget */
            SYMCONST(GL_TEXTURE_ENV),

            /* TextureGenMode */
            SYMCONST(GL_EYE_LINEAR), SYMCONST(GL_OBJECT_LINEAR),
            SYMCONST(GL_SPHERE_MAP),

            /* TextureGenParameter */
            SYMCONST(GL_TEXTURE_GEN_MODE), SYMCONST(GL_OBJECT_PLANE),
            SYMCONST(GL_EYE_PLANE),

            /* TextureMagFilter */
            SYMCONST(GL_NEAREST), SYMCONST(GL_LINEAR),

            /* TextureMinFilter */
            SYMCONST(GL_NEAREST_MIPMAP_NEAREST),
            SYMCONST(GL_LINEAR_MIPMAP_NEAREST),
            SYMCONST(GL_NEAREST_MIPMAP_LINEAR),
            SYMCONST(GL_LINEAR_MIPMAP_LINEAR),

            /* TextureParameterName */
            SYMCONST(GL_TEXTURE_MAG_FILTER), SYMCONST(GL_TEXTURE_MIN_FILTER),
            SYMCONST(GL_TEXTURE_WRAP_S), SYMCONST(GL_TEXTURE_WRAP_T),

            /* TextureTarget */
            SYMCONST(GL_PROXY_TEXTURE_1D), SYMCONST(GL_PROXY_TEXTURE_2D),

            /* TextureWrapMode */
            SYMCONST(GL_CLAMP), SYMCONST(GL_REPEAT), EndArguments);

        addSymbols(
            /* PixelInternalFormat */
            SYMCONST(GL_R3_G3_B2), SYMCONST(GL_ALPHA4), SYMCONST(GL_ALPHA8),
            SYMCONST(GL_ALPHA12), SYMCONST(GL_ALPHA16), SYMCONST(GL_LUMINANCE4),
            SYMCONST(GL_LUMINANCE8), SYMCONST(GL_LUMINANCE12),
            SYMCONST(GL_LUMINANCE16), SYMCONST(GL_LUMINANCE4_ALPHA4),
            SYMCONST(GL_LUMINANCE6_ALPHA2), SYMCONST(GL_LUMINANCE8_ALPHA8),
            SYMCONST(GL_LUMINANCE12_ALPHA4), SYMCONST(GL_LUMINANCE12_ALPHA12),
            SYMCONST(GL_LUMINANCE16_ALPHA16), SYMCONST(GL_INTENSITY),
            SYMCONST(GL_INTENSITY4), SYMCONST(GL_INTENSITY8),
            SYMCONST(GL_INTENSITY12), SYMCONST(GL_INTENSITY16),
            SYMCONST(GL_RGB4), SYMCONST(GL_RGB5), SYMCONST(GL_RGB8),
            SYMCONST(GL_RGB10), SYMCONST(GL_RGB12), SYMCONST(GL_RGB16),
            SYMCONST(GL_RGBA2), SYMCONST(GL_RGBA4), SYMCONST(GL_RGB5_A1),
            SYMCONST(GL_RGBA8), SYMCONST(GL_RGB10_A2), SYMCONST(GL_RGBA12),
            SYMCONST(GL_RGBA16),

            /* InterleavedArrayFormat */
            SYMCONST(GL_V2F), SYMCONST(GL_V3F), SYMCONST(GL_C4UB_V2F),
            SYMCONST(GL_C4UB_V3F), SYMCONST(GL_C3F_V3F), SYMCONST(GL_N3F_V3F),
            SYMCONST(GL_C4F_N3F_V3F), SYMCONST(GL_T2F_V3F),
            SYMCONST(GL_T4F_V4F), SYMCONST(GL_T2F_C4UB_V3F),
            SYMCONST(GL_T2F_C3F_V3F), SYMCONST(GL_T2F_N3F_V3F),
            SYMCONST(GL_T2F_C4F_N3F_V3F), SYMCONST(GL_T4F_C4F_N3F_V4F),

            /* ClipPlaneName */
            SYMCONST(GL_CLIP_PLANE0), SYMCONST(GL_CLIP_PLANE1),
            SYMCONST(GL_CLIP_PLANE2), SYMCONST(GL_CLIP_PLANE3),
            SYMCONST(GL_CLIP_PLANE4), SYMCONST(GL_CLIP_PLANE5),

            /* LightName */
            SYMCONST(GL_LIGHT0), SYMCONST(GL_LIGHT1), SYMCONST(GL_LIGHT2),
            SYMCONST(GL_LIGHT3), SYMCONST(GL_LIGHT4), SYMCONST(GL_LIGHT5),
            SYMCONST(GL_LIGHT6), SYMCONST(GL_LIGHT7),

            /* EXT_abgr */
            SYMCONST(GL_ABGR_EXT),

            /* EXT_blend_subtract */
            SYMCONST(GL_FUNC_SUBTRACT_EXT),
            SYMCONST(GL_FUNC_REVERSE_SUBTRACT_EXT),

        /* EXT_packed_pixels */
#ifdef GL_UNSIGNED_SHORT_4_4_4_4_EXT
            SYMCONST(GL_UNSIGNED_SHORT_4_4_4_4_EXT),
            SYMCONST(GL_UNSIGNED_SHORT_5_5_5_1_EXT),
            SYMCONST(GL_UNSIGNED_INT_8_8_8_8_EXT),
            SYMCONST(GL_UNSIGNED_INT_10_10_10_2_EXT),
#endif

            /* OpenGL12 */
            SYMCONST(GL_PACK_SKIP_IMAGES), SYMCONST(GL_PACK_IMAGE_HEIGHT),
            SYMCONST(GL_UNPACK_SKIP_IMAGES), SYMCONST(GL_UNPACK_IMAGE_HEIGHT),
            SYMCONST(GL_TEXTURE_3D), SYMCONST(GL_PROXY_TEXTURE_3D),
            SYMCONST(GL_TEXTURE_DEPTH), SYMCONST(GL_TEXTURE_WRAP_R),
            SYMCONST(GL_MAX_3D_TEXTURE_SIZE), SYMCONST(GL_BGR),
            SYMCONST(GL_BGRA), SYMCONST(GL_UNSIGNED_BYTE_3_3_2),
            SYMCONST(GL_UNSIGNED_BYTE_2_3_3_REV),
            SYMCONST(GL_UNSIGNED_SHORT_5_6_5),
            SYMCONST(GL_UNSIGNED_SHORT_5_6_5_REV),
            SYMCONST(GL_UNSIGNED_SHORT_4_4_4_4),
            SYMCONST(GL_UNSIGNED_SHORT_4_4_4_4_REV),
            SYMCONST(GL_UNSIGNED_SHORT_5_5_5_1),
            SYMCONST(GL_UNSIGNED_SHORT_1_5_5_5_REV),
            SYMCONST(GL_UNSIGNED_INT_8_8_8_8),
            SYMCONST(GL_UNSIGNED_INT_8_8_8_8_REV),
            SYMCONST(GL_UNSIGNED_INT_10_10_10_2),
            SYMCONST(GL_UNSIGNED_INT_2_10_10_10_REV),
            SYMCONST(GL_RESCALE_NORMAL), SYMCONST(GL_LIGHT_MODEL_COLOR_CONTROL),
            SYMCONST(GL_SINGLE_COLOR), SYMCONST(GL_SEPARATE_SPECULAR_COLOR),
            SYMCONST(GL_CLAMP_TO_EDGE), SYMCONST(GL_TEXTURE_MIN_LOD),
            SYMCONST(GL_TEXTURE_MAX_LOD), SYMCONST(GL_TEXTURE_BASE_LEVEL),
            SYMCONST(GL_TEXTURE_MAX_LEVEL), SYMCONST(GL_MAX_ELEMENTS_VERTICES),
            SYMCONST(GL_MAX_ELEMENTS_INDICES),
            SYMCONST(GL_ALIASED_POINT_SIZE_RANGE),
            SYMCONST(GL_ALIASED_LINE_WIDTH_RANGE), EndArguments);

        addSymbols(
            /* OpenGL13 */
            SYMCONST(GL_ACTIVE_TEXTURE), SYMCONST(GL_CLIENT_ACTIVE_TEXTURE),
            SYMCONST(GL_MAX_TEXTURE_UNITS), SYMCONST(GL_TEXTURE0),
            SYMCONST(GL_TEXTURE1), SYMCONST(GL_TEXTURE2), SYMCONST(GL_TEXTURE3),
            SYMCONST(GL_TEXTURE4), SYMCONST(GL_TEXTURE5), SYMCONST(GL_TEXTURE6),
            SYMCONST(GL_TEXTURE7), SYMCONST(GL_TEXTURE8), SYMCONST(GL_TEXTURE9),
            SYMCONST(GL_TEXTURE10), SYMCONST(GL_TEXTURE11),
            SYMCONST(GL_TEXTURE12), SYMCONST(GL_TEXTURE13),
            SYMCONST(GL_TEXTURE14), SYMCONST(GL_TEXTURE15),
            SYMCONST(GL_TEXTURE16), SYMCONST(GL_TEXTURE17),
            SYMCONST(GL_TEXTURE18), SYMCONST(GL_TEXTURE19),
            SYMCONST(GL_TEXTURE20), SYMCONST(GL_TEXTURE21),
            SYMCONST(GL_TEXTURE22), SYMCONST(GL_TEXTURE23),
            SYMCONST(GL_TEXTURE24), SYMCONST(GL_TEXTURE25),
            SYMCONST(GL_TEXTURE26), SYMCONST(GL_TEXTURE27),
            SYMCONST(GL_TEXTURE28), SYMCONST(GL_TEXTURE29),
            SYMCONST(GL_TEXTURE30), SYMCONST(GL_TEXTURE31),
            SYMCONST(GL_NORMAL_MAP), SYMCONST(GL_REFLECTION_MAP),
            SYMCONST(GL_TEXTURE_CUBE_MAP),
            SYMCONST(GL_TEXTURE_BINDING_CUBE_MAP),
            SYMCONST(GL_TEXTURE_CUBE_MAP_POSITIVE_X),
            SYMCONST(GL_TEXTURE_CUBE_MAP_NEGATIVE_X),
            SYMCONST(GL_TEXTURE_CUBE_MAP_POSITIVE_Y),
            SYMCONST(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y),
            SYMCONST(GL_TEXTURE_CUBE_MAP_POSITIVE_Z),
            SYMCONST(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z),
            SYMCONST(GL_PROXY_TEXTURE_CUBE_MAP),
            SYMCONST(GL_MAX_CUBE_MAP_TEXTURE_SIZE), SYMCONST(GL_COMBINE),
            SYMCONST(GL_COMBINE_RGB), SYMCONST(GL_COMBINE_ALPHA),
            SYMCONST(GL_RGB_SCALE), SYMCONST(GL_ADD_SIGNED),
            SYMCONST(GL_INTERPOLATE), SYMCONST(GL_CONSTANT),
            SYMCONST(GL_PRIMARY_COLOR), SYMCONST(GL_PREVIOUS),
            SYMCONST(GL_SOURCE0_RGB), SYMCONST(GL_SOURCE1_RGB),
            SYMCONST(GL_SOURCE2_RGB), SYMCONST(GL_SOURCE0_ALPHA),
            SYMCONST(GL_SOURCE1_ALPHA), SYMCONST(GL_SOURCE2_ALPHA),
            SYMCONST(GL_OPERAND0_RGB), SYMCONST(GL_OPERAND1_RGB),
            SYMCONST(GL_OPERAND2_RGB), SYMCONST(GL_OPERAND0_ALPHA),
            SYMCONST(GL_OPERAND1_ALPHA), SYMCONST(GL_OPERAND2_ALPHA),
            SYMCONST(GL_SUBTRACT), SYMCONST(GL_TRANSPOSE_MODELVIEW_MATRIX),
            SYMCONST(GL_TRANSPOSE_PROJECTION_MATRIX),
            SYMCONST(GL_TRANSPOSE_TEXTURE_MATRIX),
            SYMCONST(GL_TRANSPOSE_COLOR_MATRIX), SYMCONST(GL_COMPRESSED_ALPHA),
            SYMCONST(GL_COMPRESSED_LUMINANCE),
            SYMCONST(GL_COMPRESSED_LUMINANCE_ALPHA),
            SYMCONST(GL_COMPRESSED_INTENSITY), SYMCONST(GL_COMPRESSED_RGB),
            SYMCONST(GL_COMPRESSED_RGBA), SYMCONST(GL_TEXTURE_COMPRESSION_HINT),
            SYMCONST(GL_TEXTURE_COMPRESSED_IMAGE_SIZE),
            SYMCONST(GL_TEXTURE_COMPRESSED),
            SYMCONST(GL_NUM_COMPRESSED_TEXTURE_FORMATS),
            SYMCONST(GL_COMPRESSED_TEXTURE_FORMATS), SYMCONST(GL_DOT3_RGB),
            SYMCONST(GL_DOT3_RGBA), SYMCONST(GL_CLAMP_TO_BORDER),
            SYMCONST(GL_MULTISAMPLE), SYMCONST(GL_SAMPLE_ALPHA_TO_COVERAGE),
            SYMCONST(GL_SAMPLE_ALPHA_TO_ONE), SYMCONST(GL_SAMPLE_COVERAGE),
            SYMCONST(GL_SAMPLE_BUFFERS), SYMCONST(GL_SAMPLES),
            SYMCONST(GL_SAMPLE_COVERAGE_VALUE),
            SYMCONST(GL_SAMPLE_COVERAGE_INVERT), SYMCONST(GL_MULTISAMPLE_BIT),

        /* EXT_vertex_array */
#ifdef GL_VERTEX_ARRAY_EXT
            SYMCONST(GL_VERTEX_ARRAY_EXT), SYMCONST(GL_NORMAL_ARRAY_EXT),
            SYMCONST(GL_COLOR_ARRAY_EXT), SYMCONST(GL_INDEX_ARRAY_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_EXT),
            SYMCONST(GL_EDGE_FLAG_ARRAY_EXT),
            SYMCONST(GL_VERTEX_ARRAY_SIZE_EXT),
            SYMCONST(GL_VERTEX_ARRAY_TYPE_EXT),
            SYMCONST(GL_VERTEX_ARRAY_STRIDE_EXT),
            SYMCONST(GL_VERTEX_ARRAY_COUNT_EXT),
            SYMCONST(GL_NORMAL_ARRAY_TYPE_EXT),
            SYMCONST(GL_NORMAL_ARRAY_STRIDE_EXT),
            SYMCONST(GL_NORMAL_ARRAY_COUNT_EXT),
            SYMCONST(GL_COLOR_ARRAY_SIZE_EXT),
            SYMCONST(GL_COLOR_ARRAY_TYPE_EXT),
            SYMCONST(GL_COLOR_ARRAY_STRIDE_EXT),
            SYMCONST(GL_COLOR_ARRAY_COUNT_EXT),
            SYMCONST(GL_INDEX_ARRAY_TYPE_EXT),
            SYMCONST(GL_INDEX_ARRAY_STRIDE_EXT),
            SYMCONST(GL_INDEX_ARRAY_COUNT_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_SIZE_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_TYPE_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_STRIDE_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_COUNT_EXT),
            SYMCONST(GL_EDGE_FLAG_ARRAY_STRIDE_EXT),
            SYMCONST(GL_EDGE_FLAG_ARRAY_COUNT_EXT),
            SYMCONST(GL_VERTEX_ARRAY_POINTER_EXT),
            SYMCONST(GL_NORMAL_ARRAY_POINTER_EXT),
            SYMCONST(GL_COLOR_ARRAY_POINTER_EXT),
            SYMCONST(GL_INDEX_ARRAY_POINTER_EXT),
            SYMCONST(GL_TEXTURE_COORD_ARRAY_POINTER_EXT),
            SYMCONST(GL_EDGE_FLAG_ARRAY_POINTER_EXT),
#endif

            /* SGIS_texture_lod */
            SYMCONST(GL_TEXTURE_MIN_LOD_SGIS),
            SYMCONST(GL_TEXTURE_MAX_LOD_SGIS),
            SYMCONST(GL_TEXTURE_BASE_LEVEL_SGIS),
            SYMCONST(GL_TEXTURE_MAX_LEVEL_SGIS),

        /* EXT_shared_texture_palette */
        // SYMCONST( GL_SHARED_TEXTURE_PALETTE_EXT ),

        /* EXT_rescale_normal */
        // SYMCONST( GL_RESCALE_NORMAL_EXT ),

        /* SGIX_shadow */
#ifdef GL_TEXTURE_COMPARE_SGIX
            SYMCONST(GL_TEXTURE_COMPARE_SGIX),
            SYMCONST(GL_TEXTURE_COMPARE_OPERATOR_SGIX),
            SYMCONST(GL_TEXTURE_LEQUAL_R_SGIX),
            SYMCONST(GL_TEXTURE_GEQUAL_R_SGIX),
#endif

        /* SGIX_depth_texture */
#ifdef GL_DEPTH_COMPONENT16_SGIX
            SYMCONST(GL_DEPTH_COMPONENT16_SGIX),
            SYMCONST(GL_DEPTH_COMPONENT24_SGIX),
            SYMCONST(GL_DEPTH_COMPONENT32_SGIX),
#endif

            /* SGIS_generate_mipmap */
            SYMCONST(GL_GENERATE_MIPMAP_SGIS),
            SYMCONST(GL_GENERATE_MIPMAP_HINT_SGIS),

            /* OpenGL14 */
            SYMCONST(GL_POINT_SIZE_MIN), SYMCONST(GL_POINT_SIZE_MAX),
            SYMCONST(GL_POINT_FADE_THRESHOLD_SIZE),
            SYMCONST(GL_POINT_DISTANCE_ATTENUATION),
            SYMCONST(GL_FOG_COORDINATE_SOURCE), SYMCONST(GL_FOG_COORDINATE),
            SYMCONST(GL_FRAGMENT_DEPTH), SYMCONST(GL_CURRENT_FOG_COORDINATE),
            SYMCONST(GL_FOG_COORDINATE_ARRAY_TYPE),
            SYMCONST(GL_FOG_COORDINATE_ARRAY_STRIDE),
            SYMCONST(GL_FOG_COORDINATE_ARRAY_POINTER),
            SYMCONST(GL_FOG_COORDINATE_ARRAY), SYMCONST(GL_COLOR_SUM),
            SYMCONST(GL_CURRENT_SECONDARY_COLOR),
            SYMCONST(GL_SECONDARY_COLOR_ARRAY_SIZE),
            SYMCONST(GL_SECONDARY_COLOR_ARRAY_TYPE),
            SYMCONST(GL_SECONDARY_COLOR_ARRAY_STRIDE),
            SYMCONST(GL_SECONDARY_COLOR_ARRAY_POINTER),
            SYMCONST(GL_SECONDARY_COLOR_ARRAY), SYMCONST(GL_INCR_WRAP),
            SYMCONST(GL_DECR_WRAP), SYMCONST(GL_MAX_TEXTURE_LOD_BIAS),
            SYMCONST(GL_TEXTURE_FILTER_CONTROL), SYMCONST(GL_TEXTURE_LOD_BIAS),
            SYMCONST(GL_GENERATE_MIPMAP), SYMCONST(GL_GENERATE_MIPMAP_HINT),
            SYMCONST(GL_BLEND_DST_RGB), SYMCONST(GL_BLEND_SRC_RGB),
            SYMCONST(GL_BLEND_DST_ALPHA), SYMCONST(GL_BLEND_SRC_ALPHA),
            SYMCONST(GL_MIRRORED_REPEAT), SYMCONST(GL_DEPTH_COMPONENT16),
            SYMCONST(GL_DEPTH_COMPONENT24), SYMCONST(GL_DEPTH_COMPONENT32),
            SYMCONST(GL_TEXTURE_DEPTH_SIZE), SYMCONST(GL_DEPTH_TEXTURE_MODE),
            SYMCONST(GL_TEXTURE_COMPARE_MODE),
            SYMCONST(GL_TEXTURE_COMPARE_FUNC),
            SYMCONST(GL_COMPARE_R_TO_TEXTURE), EndArguments);

        addSymbols(

            new Function(c, "glAccum", GLModule::glAccum, None, Return, "void",
                         Args, "int", "float", End),

            new Function(c, "glAlphaFunc", GLModule::glAlphaFunc, None, Return,
                         "void", Args, "int", "float", End),

            new Function(c, "glArrayElement", GLModule::glArrayElement, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glBegin", GLModule::glBegin, None, Return, "void",
                         Args, "int", End),

            new Function(c, "glBindTexture", GLModule::glBindTexture, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glBlendFunc", GLModule::glBlendFunc, None, Return,
                         "void", Args, "int", "int", End),

            new Function(c, "glCallList", GLModule::glCallList, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glClear", GLModule::glClear, None, Return, "void",
                         Args, "int", End),

            new Function(c, "glClearAccum", GLModule::glClearAccum, None,
                         Return, "void", Args, "float", "float", "float",
                         "float", End),

            new Function(c, "glClearColor", GLModule::glClearColor, None,
                         Return, "void", Args, "float", "float", "float",
                         "float", End),

            new Function(c, "glClearDepth", GLModule::glClearDepth, None,
                         Return, "void", Args, "float", End),

            new Function(c, "glClearIndex", GLModule::glClearIndex, None,
                         Return, "void", Args, "float", End),

            new Function(c, "glClearStencil", GLModule::glClearStencil, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glClipPlane", GLModule::glClipPlane, None, Return,
                         "void", Args, "int", "float", "float", "float",
                         "float", End),

            new Function(c, "glColor", GLModule::glColor3f, None, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "glColor", GLModule::glColor4f, None, Return,
                         "void", Args, "float", "float", "float", "float", End),

            new Function(c, "glColor", GLModule::glColor3fv, None, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "glColor", GLModule::glColor4fv, None, Return,
                         "void", Args, "vector float[4]", End),

            new Function(c, "glColorMask", GLModule::glColorMask, None, Return,
                         "void", Args, "bool", "bool", "bool", "bool", End),

            new Function(c, "glColorMaterial", GLModule::glColorMaterial, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glCopyPixels", GLModule::glCopyPixels, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", End),

            new Function(c, "glCopyTexImage1D", GLModule::glCopyTexImage1D,
                         None, Return, "void", Args, "int", "int", "int", "int",
                         "int", "int", "int", End),

            new Function(c, "glCopyTexImage2D", GLModule::glCopyTexImage2D,
                         None, Return, "void", Args, "int", "int", "int", "int",
                         "int", "int", "int", "int", End),

            new Function(c, "glCopyTexSubImage1D",
                         GLModule::glCopyTexSubImage1D, None, Return, "void",
                         Args, "int", "int", "int", "int", "int", "int", End),

            new Function(c, "glCopyTexSubImage2D",
                         GLModule::glCopyTexSubImage2D, None, Return, "void",
                         Args, "int", "int", "int", "int", "int", "int", "int",
                         "int", End),

            new Function(c, "glCullFace", GLModule::glCullFace, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glDeleteLists", GLModule::glDeleteLists, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glDeleteTextures", GLModule::glDeleteTextures,
                         None, Return, "void", Args, "int[]", End),

            new Function(c, "glDepthFunc", GLModule::glDepthFunc, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glDepthMask", GLModule::glDepthMask, None, Return,
                         "void", Args, "bool", End),

            new Function(c, "glDepthRange", GLModule::glDepthRange, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glDisable", GLModule::glDisable, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glDisableClientState",
                         GLModule::glDisableClientState, None, Return, "void",
                         Args, "int", End),

            new Function(c, "glDrawArrays", GLModule::glDrawArrays, None,
                         Return, "void", Args, "int", "int", "int", End),

            new Function(c, "glDrawBuffer", GLModule::glDrawBuffer, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glDrawElements", GLModule::glDrawElements, None,
                         Return, "void", Args, "int", "int", "int[]", End),

            new Function(c, "glDrawPixels", GLModule::glDrawPixels, None,
                         Return, "void", Args, "int", "int", "int",
                         "?dyn_array", End),

            new Function(c, "glEdgeFlag", GLModule::glEdgeFlag, None, Return,
                         "void", Args, "bool", End),

            new Function(c, "glEnable", GLModule::glEnable, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glEnableClientState",
                         GLModule::glEnableClientState, None, Return, "void",
                         Args, "int", End),

            new Function(c, "glEnd", GLModule::glEnd, None, Return, "void",
                         End),

            new Function(c, "glEndList", GLModule::glEndList, None, Return,
                         "void", End),

            new Function(c, "glEvalCoord1f", GLModule::glEvalCoord1f, None,
                         Return, "void", Args, "float", End),

            new Function(c, "glEvalCoord2f", GLModule::glEvalCoord2f, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glEvalMesh1", GLModule::glEvalMesh1, None, Return,
                         "void", Args, "int", "int", "int", End),

            new Function(c, "glEvalMesh2", GLModule::glEvalMesh2, None, Return,
                         "void", Args, "int", "int", "int", "int", "int", End),

            new Function(c, "glEvalPoint1", GLModule::glEvalPoint1, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glEvalPoint2", GLModule::glEvalPoint2, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glFinish", GLModule::glFinish, None, Return,
                         "void", End),

            new Function(c, "glFlush", GLModule::glFlush, None, Return, "void",
                         End),

            new Function(c, "glFog", GLModule::glFogfv, None, Return, "void",
                         Args, "int", "float[]", End),

            new Function(c, "glFog", GLModule::glFogiv, None, Return, "void",
                         Args, "int", "int[]", End),

            new Function(c, "glFog", GLModule::glFogf, None, Return, "void",
                         Args, "int", "float", End),

            new Function(c, "glFog", GLModule::glFogi, None, Return, "void",
                         Args, "int", "int", End),

            new Function(c, "glFrontFace", GLModule::glFrontFace, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glFrustum", GLModule::glFrustum, None, Return,
                         "void", Args, "float", "float", "float", "float",
                         "float", "float", End),

            new Function(c, "glGenLists", GLModule::glGenLists, None, Return,
                         "int", Args, "int", End),

            new Function(c, "glGenTextures", GLModule::glGenTextures, None,
                         Return, "int[]", Args, "int", End),

            new Function(c, "glGetBoolean", GLModule::glGetBooleanv, None,
                         Return, "bool[]", Args, "int", End),

            new Function(c, "glGetClipPlane", GLModule::glGetClipPlane, None,
                         Return, "float[4]", Args, "int", End),

            new Function(c, "glGetError", GLModule::glGetError, None, Return,
                         "int", End),

            new Function(c, "glGetFloat", GLModule::glGetFloatv, None, Return,
                         "float[]", Args, "int", End),

            new Function(c, "glGetInteger", GLModule::glGetIntegerv, None,
                         Return, "int[]", Args, "int", End),

            new Function(c, "glGetString", GLModule::glGetString, None, Return,
                         "string", Args, "int", End),

            new Function(c, "glHint", GLModule::glHint, None, Return, "void",
                         Args, "int", "int", End),

            new Function(c, "glIndexMask", GLModule::glIndexMask, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glIndex", GLModule::glIndexf, None, Return, "void",
                         Args, "float", End),

            new Function(c, "glIndex", GLModule::glIndexi, None, Return, "void",
                         Args, "int", End),

            new Function(c, "glInitNames", GLModule::glInitNames, None, Return,
                         "void", End),

            new Function(c, "glIsEnabled", GLModule::glIsEnabled, None, Return,
                         "bool", Args, "int", End),

            new Function(c, "glIsList", GLModule::glIsList, None, Return,
                         "bool", Args, "int", End),

            new Function(c, "glIsTexture", GLModule::glIsTexture, None, Return,
                         "bool", Args, "int", End),

            new Function(c, "glLightModel", GLModule::glLightModelf, None,
                         Return, "void", Args, "int", "float", End),

            new Function(c, "glLightModel", GLModule::glLightModeli, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glLight", GLModule::glLightf, None, Return, "void",
                         Args, "int", "int", "float", End),

            new Function(c, "glLight", GLModule::glLighti, None, Return, "void",
                         Args, "int", "int", "int", End),

            new Function(c, "glLineStipple", GLModule::glLineStipple, None,
                         Return, "void", Args, "int", "short", End),

            new Function(c, "glLineWidth", GLModule::glLineWidth, None, Return,
                         "void", Args, "float", End),

            new Function(c, "glListBase", GLModule::glListBase, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glLoadIdentity", GLModule::glLoadIdentity, None,
                         Return, "void", End),

            new Function(c, "glLoadMatrix", GLModule::glLoadMatrixf, None,
                         Return, "void", Args, "float[4,4]", End),

            new Function(c, "glLoadName", GLModule::glLoadName, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glLogicOp", GLModule::glLogicOp, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glMapGrid1", GLModule::glMapGrid1f, None, Return,
                         "void", Args, "int", "float", "float", End),

            new Function(c, "glMapGrid2", GLModule::glMapGrid2f, None, Return,
                         "void", Args, "int", "float", "float", "int", "float",
                         "float", End),

            new Function(c, "glMaterial", GLModule::glMaterialf, None, Return,
                         "void", Args, "int", "int", "float", End),

            new Function(c, "glMaterial", GLModule::glMateriali, None, Return,
                         "void", Args, "int", "int", "int", End),

            new Function(c, "glMatrixMode", GLModule::glMatrixMode, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glMultMatrix", GLModule::glMultMatrixf, None,
                         Return, "void", Args, "float[4,4]", End),

            new Function(c, "glNewList", GLModule::glNewList, None, Return,
                         "void", Args, "int", "int", End),

            new Function(c, "glNormal", GLModule::glNormal3f, None, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "glNormal", GLModule::glNormal3fv, None, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "glOrtho", GLModule::glOrtho, None, Return, "void",
                         Args, "float", "float", "float", "float", "float",
                         "float", End),

            new Function(c, "glPassThrough", GLModule::glPassThrough, None,
                         Return, "void", Args, "float", End),

            new Function(c, "glPixelStore", GLModule::glPixelStoref, None,
                         Return, "void", Args, "int", "float", End),

            new Function(c, "glPixelStore", GLModule::glPixelStorei, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glPixelTransfer", GLModule::glPixelTransferf, None,
                         Return, "void", Args, "int", "float", End),

            new Function(c, "glPixelTransfer", GLModule::glPixelTransferi, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glPixelZoom", GLModule::glPixelZoom, None, Return,
                         "void", Args, "float", "float", End),

            new Function(c, "glPointSize", GLModule::glPointSize, None, Return,
                         "void", Args, "float", End),

            new Function(c, "glPolygonMode", GLModule::glPolygonMode, None,
                         Return, "void", Args, "int", "int", End),

            new Function(c, "glPolygonOffset", GLModule::glPolygonOffset, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glPolygonStipple", GLModule::glPolygonStipple,
                         None, Return, "void", Args, "byte[32,32]", End),

            new Function(c, "glPopAttrib", GLModule::glPopAttrib, None, Return,
                         "void", End),

            new Function(c, "glPopClientAttrib", GLModule::glPopClientAttrib,
                         None, Return, "void", End),

            new Function(c, "glPopMatrix", GLModule::glPopMatrix, None, Return,
                         "void", End),

            new Function(c, "glPopName", GLModule::glPopName, None, Return,
                         "void", End),

            new Function(c, "glPushAttrib", GLModule::glPushAttrib, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glPushClientAttrib", GLModule::glPushClientAttrib,
                         None, Return, "void", Args, "int", End),

            new Function(c, "glPushMatrix", GLModule::glPushMatrix, None,
                         Return, "void", End),

            new Function(c, "glPushName", GLModule::glPushName, None, Return,
                         "void", Args, "int", End),

            new Function(c, "glRasterPos", GLModule::glRasterPos2f, None,
                         Return, "void", Args, "float", "float", End),

            new Function(c, "glRasterPos", GLModule::glRasterPos2fv, None,
                         Return, "void", Args, "vector float[2]", End),

            new Function(c, "glRasterPos", GLModule::glRasterPos3f, None,
                         Return, "void", Args, "float", "float", "float", End),

            new Function(c, "glRasterPos", GLModule::glRasterPos3fv, None,
                         Return, "void", Args, "vector float[3]", End),

            new Function(c, "glReadBuffer", GLModule::glReadBuffer, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glReadPixels", GLModule::glReadPixels, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", "?dyn_array", End),

            new Function(c, "glRenderMode", GLModule::glRenderMode, None,
                         Return, "int", Args, "int", End),

            new Function(c, "glRotate", GLModule::glRotatef, None, Return,
                         "void", Args, "float", "float", "float", "float", End),

            new Function(c, "glScale", GLModule::glScalef, None, Return, "void",
                         Args, "float", "float", "float", End),

            new Function(c, "glScissor", GLModule::glScissor, None, Return,
                         "void", Args, "int", "int", "int", "int", End),

            new Function(c, "glShadeModel", GLModule::glShadeModel, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glStencilFunc", GLModule::glStencilFunc, None,
                         Return, "void", Args, "int", "int", "int", End),

            new Function(c, "glStencilMask", GLModule::glStencilMask, None,
                         Return, "void", Args, "int", End),

            new Function(c, "glStencilOp", GLModule::glStencilOp, None, Return,
                         "void", Args, "int", "int", "int", End),

            new Function(c, "glTexCoord", GLModule::glTexCoord1f, None, Return,
                         "void", Args, "float", End),

            new Function(c, "glTexCoord", GLModule::glTexCoord2f, None, Return,
                         "void", Args, "float", "float", End),

            new Function(c, "glTexCoord", GLModule::glTexCoord2fv, None, Return,
                         "void", Args, "vector float[2]", End),

            new Function(c, "glTexCoord", GLModule::glTexCoord3f, None, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "glTexCoord", GLModule::glTexCoord3fv, None, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "glTexEnv", GLModule::glTexEnvf, None, Return,
                         "void", Args, "int", "int", "float", End),

            new Function(c, "glTexEnv", GLModule::glTexEnvi, None, Return,
                         "void", Args, "int", "int", "int", End),

            new Function(c, "glTexGen", GLModule::glTexGenf, None, Return,
                         "void", Args, "int", "int", "float", End),

            new Function(c, "glTexGen", GLModule::glTexGeni, None, Return,
                         "void", Args, "int", "int", "int", End),

            new Function(c, "glTexImage1D", GLModule::glTexImage1D, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", "int", "?dyn_array", End),

            new Function(c, "glTexImage2D", GLModule::glTexImage2D, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", "int", "int", "?dyn_array", End),

            new Function(c, "glTexParameter", GLModule::glTexParameterf, None,
                         Return, "void", Args, "int", "int", "float", End),

            new Function(c, "glTexParameter", GLModule::glTexParameteri, None,
                         Return, "void", Args, "int", "int", "int", End),

            new Function(c, "glTexSubImage1D", GLModule::glTexSubImage1D, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", "?dyn_array", End),

            new Function(c, "glTexSubImage2D", GLModule::glTexSubImage2D, None,
                         Return, "void", Args, "int", "int", "int", "int",
                         "int", "int", "int", "?dyn_array", End),

            new Function(c, "glTranslate", GLModule::glTranslatef, None, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "glViewport", GLModule::glViewport, None, Return,
                         "void", Args, "int", "int", "int", "int", End),

            new Function(c, "glVertex", GLModule::glVertex2f, None, Return,
                         "void", Args, "float", "float", End),

            new Function(c, "glVertex", GLModule::glVertex3f, None, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "glVertex", GLModule::glVertex2fv, None, Return,
                         "void", Args, "vector float[2]", End),

            new Function(c, "glVertex", GLModule::glVertex3fv, None, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "glVertexPointer", GLModule::glVertexPointer4fv,
                         None, Return, "void", Args, "(vector float[4])[]",
                         End),

            new Function(c, "glVertexPointer", GLModule::glVertexPointer3fv,
                         None, Return, "void", Args, "(vector float[3])[]",
                         End),

            new Function(c, "glVertexPointer", GLModule::glVertexPointer2fv,
                         None, Return, "void", Args, "(vector float[2])[]",
                         End),

            new Function(c, "glNormalPointer", GLModule::glNormalPointer3fv,
                         None, Return, "void", Args, "(vector float[3])[]",
                         End),

            new Function(c, "glColorPointer", GLModule::glColorPointer4fv, None,
                         Return, "void", Args, "(vector float[4])[]", End),

            new Function(c, "glColorPointer", GLModule::glColorPointer3fv, None,
                         Return, "void", Args, "(vector float[3])[]", End),

            new Function(c, "glIndexPointer", GLModule::glIndexPointer, None,
                         Return, "void", Args, "int[]", End),

            new Function(c, "glTexCoordPointer", GLModule::glTexCoordPointer4fv,
                         None, Return, "void", Args, "(vector float[4])[]",
                         End),

            new Function(c, "glTexCoordPointer", GLModule::glTexCoordPointer3fv,
                         None, Return, "void", Args, "(vector float[3])[]",
                         End),

            new Function(c, "glTexCoordPointer", GLModule::glTexCoordPointer2fv,
                         None, Return, "void", Args, "(vector float[2])[]",
                         End),

            new Function(c, "glTexCoordPointer", GLModule::glTexCoordPointer1f,
                         None, Return, "void", Args, "float[]", End),

            new Function(c, "glInterleavedArrays",
                         GLModule::glInterleavedArrays, None, Return, "void",
                         Args, "int", "int", "?dyn_array", End),

            EndArguments);
    }

    // *****************************************************************************

    NODE_IMPLEMENTATION(GLModule::glAccum, void)
    {
        ::glAccum(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glAlphaFunc, void)
    {
        ::glAlphaFunc(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glArrayElement, void)
    {
        ::glArrayElement(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glBegin, void)
    {
        ::glBegin(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glBindTexture, void)
    {
        ::glBindTexture(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glBlendFunc, void)
    {
        ::glBlendFunc(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCallList, void)
    {
        ::glCallList(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glClear, void)
    {
        ::glClear(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glClearAccum, void)
    {
        ::glClearAccum(NODE_ARG(0, float), NODE_ARG(1, float),
                       NODE_ARG(2, float), NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLModule::glClearColor, void)
    {
        ::glClearColor(NODE_ARG(0, float), NODE_ARG(1, float),
                       NODE_ARG(2, float), NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLModule::glClearDepth, void)
    {
        ::glClearDepth(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glClearIndex, void)
    {
        ::glClearIndex(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glClearStencil, void)
    {
        ::glClearStencil(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glClipPlane, void)
    {
        double clipPlane[4];
        clipPlane[0] = NODE_ARG(1, float);
        clipPlane[1] = NODE_ARG(2, float);
        clipPlane[2] = NODE_ARG(3, float);
        clipPlane[3] = NODE_ARG(4, float);
        ::glClipPlane(NODE_ARG(0, int), clipPlane);
    }

    NODE_IMPLEMENTATION(GLModule::glColor3f, void)
    {
        ::glColor3f(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glColor3fv, void)
    {
        const Vector3f& v = NODE_ARG(0, Vector3f);
        ::glColor3f(v[0], v[1], v[2]);
    }

    NODE_IMPLEMENTATION(GLModule::glColor4f, void)
    {
        ::glColor4f(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                    NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLModule::glColor4fv, void)
    {
        const Vector4f& v = NODE_ARG(0, Vector4f);
        ::glColor4f(v[0], v[1], v[2], v[3]);
    }

    NODE_IMPLEMENTATION(GLModule::glColorMask, void)
    {
        ::glColorMask(NODE_ARG(0, bool), NODE_ARG(1, bool), NODE_ARG(2, bool),
                      NODE_ARG(3, bool));
    }

    NODE_IMPLEMENTATION(GLModule::glColorMaterial, void)
    {
        ::glColorMaterial(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCopyPixels, void)
    {
        ::glCopyPixels(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                       NODE_ARG(3, int), NODE_ARG(4, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCopyTexImage1D, void)
    {
        ::glCopyTexImage1D(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                           NODE_ARG(3, int), NODE_ARG(4, int), NODE_ARG(5, int),
                           NODE_ARG(6, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCopyTexImage2D, void)
    {
        ::glCopyTexImage2D(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                           NODE_ARG(3, int), NODE_ARG(4, int), NODE_ARG(5, int),
                           NODE_ARG(6, int), NODE_ARG(7, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCopyTexSubImage1D, void)
    {
        ::glCopyTexSubImage1D(NODE_ARG(0, int), NODE_ARG(1, int),
                              NODE_ARG(2, int), NODE_ARG(3, int),
                              NODE_ARG(4, int), NODE_ARG(5, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCopyTexSubImage2D, void)
    {
        ::glCopyTexSubImage2D(NODE_ARG(0, int), NODE_ARG(1, int),
                              NODE_ARG(2, int), NODE_ARG(3, int),
                              NODE_ARG(4, int), NODE_ARG(5, int),
                              NODE_ARG(6, int), NODE_ARG(7, int));
    }

    NODE_IMPLEMENTATION(GLModule::glCullFace, void)
    {
        ::glCullFace(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDeleteLists, void)
    {
        ::glDeleteLists(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDeleteTextures, void)
    {
        DynamicArray* t = NODE_ARG_OBJECT(0, DynamicArray);
        ::glDeleteTextures(t->size(), t->data<const GLuint>());
    }

    NODE_IMPLEMENTATION(GLModule::glDepthFunc, void)
    {
        ::glDepthFunc(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDepthMask, void)
    {
        ::glDepthMask(NODE_ARG(0, bool));
    }

    NODE_IMPLEMENTATION(GLModule::glDepthRange, void)
    {
        ::glDepthRange(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glDisable, void)
    {
        ::glDisable(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDisableClientState, void)
    {
        ::glDisableClientState(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDrawArrays, void)
    {
        ::glDrawArrays(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDrawBuffer, void)
    {
        ::glDrawBuffer(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glDrawElements, void)
    {
        int mode = NODE_ARG(0, int);
        int count = NODE_ARG(1, int);
        DynamicArray* indices = NODE_ARG_OBJECT(2, DynamicArray);
        ::glDrawElements(mode, count, GL_UNSIGNED_INT, indices->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glDrawPixels, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int width = NODE_ARG(0, int);
        int height = NODE_ARG(1, int);
        int format = NODE_ARG(2, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(3, DynamicArray);
        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType() || t == c->vec3fType() || t == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        ::glDrawPixels(width, height, format, type, pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glEdgeFlag, void)
    {
        ::glEdgeFlag(NODE_ARG(0, bool));
    }

    NODE_IMPLEMENTATION(GLModule::glEnable, void)
    {
        ::glEnable(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glEnableClientState, void)
    {
        ::glEnableClientState(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glEnd, void) { ::glEnd(); }

    NODE_IMPLEMENTATION(GLModule::glEndList, void) { ::glEndList(); }

    NODE_IMPLEMENTATION(GLModule::glEvalCoord1f, void)
    {
        ::glEvalCoord1f(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glEvalCoord2f, void)
    {
        ::glEvalCoord2f(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glEvalMesh1, void)
    {
        ::glEvalMesh1(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glEvalMesh2, void)
    {
        ::glEvalMesh2(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                      NODE_ARG(3, int), NODE_ARG(4, int));
    }

    NODE_IMPLEMENTATION(GLModule::glEvalPoint1, void)
    {
        ::glEvalPoint1(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glEvalPoint2, void)
    {
        ::glEvalPoint2(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glFinish, void) { ::glFinish(); }

    NODE_IMPLEMENTATION(GLModule::glFlush, void) { ::glFlush(); }

    NODE_IMPLEMENTATION(GLModule::glFogfv, void)
    {
        int mode = NODE_ARG(0, int);
        ::glFogfv(mode, NODE_ARG_OBJECT(1, DynamicArray)->data<float>());
    }

    NODE_IMPLEMENTATION(GLModule::glFogiv, void)
    {
        int mode = NODE_ARG(0, int);
        ::glFogiv(mode, NODE_ARG_OBJECT(1, DynamicArray)->data<const GLint>());
    }

    NODE_IMPLEMENTATION(GLModule::glFogf, void)
    {
        ::glFogf(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glFogi, void)
    {
        ::glFogi(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glFrontFace, void)
    {
        ::glFrontFace(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glFrustum, void)
    {
        ::glFrustum(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                    NODE_ARG(3, float), NODE_ARG(4, float), NODE_ARG(5, float));
    }

    NODE_IMPLEMENTATION(GLModule::glGenLists, int)
    {
        NODE_RETURN(::glGenLists(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(GLModule::glGetBooleanv, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const DynamicArrayType* atype =
            (DynamicArrayType*)c->arrayType(c->boolType(), 1, 0);
        DynamicArray* values = new DynamicArray(atype, 1);
        values->resize(16);

        int tmpInts[16];
        for (int i = 0; i < 16; ++i)
        {
            tmpInts[i] = std::numeric_limits<int>::max();
        }

        ::glGetIntegerv(NODE_ARG(0, int), (GLint*)tmpInts);

        for (int i = 0; i < 16; ++i)
        {
            if (tmpInts[i] == std::numeric_limits<int>::max())
            {
                values->resize(i);
                break;
            }
            values->element<bool>(i) = (bool)tmpInts[i];
        }

        NODE_RETURN(values);
    }

    NODE_IMPLEMENTATION(GLModule::glGenTextures, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        int numTextures = NODE_ARG(0, int);

        unsigned int* textures = new unsigned int[numTextures];
        ::glGenTextures(numTextures, (GLuint*)textures);

        const DynamicArrayType* atype =
            (DynamicArrayType*)c->arrayType(c->intType(), 1, 0);
        DynamicArray* texArray = new DynamicArray(atype, 1);
        texArray->resize(numTextures);

        for (int i = 0; i < numTextures; ++i)
        {
            texArray->element<int>(i) = textures[i];
        }
        delete[] textures;
        NODE_RETURN(texArray);
    }

    NODE_IMPLEMENTATION(GLModule::glGetClipPlane, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        double clipPlane[4];
        ::glGetClipPlane(NODE_ARG(0, int), clipPlane);
        const FixedArrayType* atype =
            (FixedArrayType*)c->arrayType(c->floatType(), 1, 4);
        FixedArray* cp = (FixedArray*)ClassInstance::allocate(atype);

        cp->element<float>(0) = clipPlane[0];
        cp->element<float>(1) = clipPlane[1];
        cp->element<float>(2) = clipPlane[2];
        cp->element<float>(3) = clipPlane[3];
        NODE_RETURN(cp);
    }

    NODE_IMPLEMENTATION(GLModule::glGetError, int)
    {
        NODE_RETURN(::glGetError());
    }

    NODE_IMPLEMENTATION(GLModule::glGetFloatv, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const DynamicArrayType* atype =
            (DynamicArrayType*)c->arrayType(c->floatType(), 1, 0);
        DynamicArray* values = new DynamicArray(atype, 1);

        values->resize(16);
        for (int i = 0; i < 16; ++i)
        {
            /* AJG - wtf mf? */
            values->element<float>(i) = 0;
        }

        ::glGetFloatv(NODE_ARG(0, int), values->data<float>());

        for (int i = 0; i < 16; ++i)
        {
            if (isnan(values->element<float>(i)))
            {
                values->resize(i);
                break;
            }
        }

        NODE_RETURN(values);
    }

    NODE_IMPLEMENTATION(GLModule::glGetIntegerv, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const DynamicArrayType* atype =
            (DynamicArrayType*)c->arrayType(c->intType(), 1, 0);
        DynamicArray* values = new DynamicArray(atype, 1);

        values->resize(16);
        for (int i = 0; i < 16; ++i)
        {
            values->element<int>(i) = std::numeric_limits<int>::max();
        }

        ::glGetIntegerv(NODE_ARG(0, int), values->data<GLint>());

        for (int i = 0; i < 16; ++i)
        {
            if (values->element<int>(i) == std::numeric_limits<int>::max())
            {
                values->resize(i);
                break;
            }
        }

        NODE_RETURN(values);
    }

    NODE_IMPLEMENTATION(GLModule::glGetString, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        if (const unsigned char* str = ::glGetString(NODE_ARG(0, int)))
        {
            NODE_RETURN(c->stringType()->allocate((const char*)str));
        }
        else
        {
            throw BadArgumentException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(GLModule::glHint, void)
    {
        ::glHint(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glIndexMask, void)
    {
        ::glIndexMask(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glIndexf, void)
    {
        ::glIndexf(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glIndexi, void)
    {
        ::glIndexi(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glInitNames, void) { ::glInitNames(); }

    NODE_IMPLEMENTATION(GLModule::glIsEnabled, bool)
    {
        NODE_RETURN(::glIsEnabled(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(GLModule::glIsList, bool)
    {
        NODE_RETURN(::glIsList(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(GLModule::glIsTexture, bool)
    {
        NODE_RETURN(::glIsTexture(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(GLModule::glLightModelf, void)
    {
        ::glLightModelf(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glLightModeli, void)
    {
        ::glLightModeli(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glLightf, void)
    {
        ::glLightf(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glLighti, void)
    {
        ::glLightf(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glLineStipple, void)
    {
        ::glLineStipple(NODE_ARG(0, int), NODE_ARG(1, short));
    }

    NODE_IMPLEMENTATION(GLModule::glLineWidth, void)
    {
        ::glLineWidth(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glListBase, void)
    {
        ::glListBase(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glLoadIdentity, void) { ::glLoadIdentity(); }

    NODE_IMPLEMENTATION(GLModule::glLoadMatrixf, void)
    {
        FixedArray* t = NODE_ARG_OBJECT(0, FixedArray);
        ::glLoadMatrixf(t->data<float>());
    }

    NODE_IMPLEMENTATION(GLModule::glLoadName, void)
    {
        ::glLoadName(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glLogicOp, void)
    {
        ::glLogicOp(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glMapGrid1f, void)
    {
        ::glMapGrid1f(NODE_ARG(0, int), NODE_ARG(1, float), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glMapGrid2f, void)
    {
        ::glMapGrid2f(NODE_ARG(0, int), NODE_ARG(1, float), NODE_ARG(2, float),
                      NODE_ARG(3, int), NODE_ARG(4, float), NODE_ARG(5, float));
    }

    NODE_IMPLEMENTATION(GLModule::glMaterialf, void)
    {
        ::glMaterialf(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glMateriali, void)
    {
        ::glMateriali(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glMatrixMode, void)
    {
        ::glMatrixMode(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glMultMatrixf, void)
    {
        FixedArray* t = NODE_ARG_OBJECT(0, FixedArray);
        ::glMultMatrixf(t->data<float>());
    }

    NODE_IMPLEMENTATION(GLModule::glNewList, void)
    {
        ::glNewList(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glNormal3f, void)
    {
        ::glNormal3f(NODE_ARG(0, float), NODE_ARG(1, float),
                     NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glNormal3fv, void)
    {
        const Vector3f& v = NODE_ARG(0, Vector3f);
        ::glNormal3f(v[0], v[1], v[2]);
    }

    NODE_IMPLEMENTATION(GLModule::glOrtho, void)
    {
        ::glOrtho(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                  NODE_ARG(3, float), NODE_ARG(4, float), NODE_ARG(5, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPassThrough, void)
    {
        ::glPassThrough(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPixelStoref, void)
    {
        ::glPixelStoref(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPixelStorei, void)
    {
        ::glPixelStorei(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glPixelTransferf, void)
    {
        ::glPixelTransferf(NODE_ARG(0, int), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPixelTransferi, void)
    {
        ::glPixelTransferi(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glPixelZoom, void)
    {
        ::glPixelZoom(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPointSize, void)
    {
        ::glPointSize(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPolygonMode, void)
    {
        ::glPolygonMode(NODE_ARG(0, int), NODE_ARG(1, int));
    }

    NODE_IMPLEMENTATION(GLModule::glPolygonOffset, void)
    {
        ::glPolygonOffset(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glPolygonStipple, void)
    {
        FixedArray* array = NODE_ARG_OBJECT(0, FixedArray);
        ::glPolygonStipple(array->elementPointer(0));
    }

    NODE_IMPLEMENTATION(GLModule::glPopAttrib, void) { ::glPopAttrib(); }

    NODE_IMPLEMENTATION(GLModule::glPopClientAttrib, void)
    {
        ::glPopClientAttrib();
    }

    NODE_IMPLEMENTATION(GLModule::glPopMatrix, void) { ::glPopMatrix(); }

    NODE_IMPLEMENTATION(GLModule::glPopName, void) { ::glPopName(); }

    NODE_IMPLEMENTATION(GLModule::glPushAttrib, void)
    {
        ::glPushAttrib(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glPushClientAttrib, void)
    {
        ::glPushClientAttrib(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glPushMatrix, void) { ::glPushMatrix(); }

    NODE_IMPLEMENTATION(GLModule::glPushName, void)
    {
        ::glPushName(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glRasterPos2f, void)
    {
        ::glRasterPos2f(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glRasterPos2fv, void)
    {
        const Vector2f& v = NODE_ARG(0, Vector2f);
        ::glRasterPos2f(v[0], v[1]);
    }

    NODE_IMPLEMENTATION(GLModule::glRasterPos3f, void)
    {
        ::glRasterPos3f(NODE_ARG(0, float), NODE_ARG(1, float),
                        NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glRasterPos3fv, void)
    {
        const Vector3f& v = NODE_ARG(0, Vector3f);
        ::glRasterPos3f(v[0], v[1], v[2]);
    }

    NODE_IMPLEMENTATION(GLModule::glReadBuffer, void)
    {
        ::glReadBuffer(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glReadPixels, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int x = NODE_ARG(0, int);
        int y = NODE_ARG(1, int);
        int width = NODE_ARG(2, int);
        int height = NODE_ARG(3, int);
        int format = NODE_ARG(4, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(5, DynamicArray);

        if (!pixelArray)
        {
            throw BadArgumentException(NODE_THREAD);
        }

        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        int numElements = 0;
        switch (format)
        {
        case GL_COLOR_INDEX:
        case GL_STENCIL_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_LUMINANCE:
            numElements = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            numElements = 2;
            break;
        case GL_RGB:
        case GL_BGR:
            numElements = 3;
            break;
        case GL_RGBA:
        case GL_BGRA:
            numElements = 4;
            break;
        default:
            throw BadArgumentException(NODE_THREAD);
        }

        pixelArray->resize(width * height * numElements);

        ::glReadPixels(x, y, width, height, format, type,
                       pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glRenderMode, int)
    {
        NODE_RETURN(::glRenderMode(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(GLModule::glRotatef, void)
    {
        ::glRotatef(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                    NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLModule::glScalef, void)
    {
        ::glScalef(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glScissor, void)
    {
        ::glScissor(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                    NODE_ARG(3, int));
    }

    NODE_IMPLEMENTATION(GLModule::glShadeModel, void)
    {
        ::glShadeModel(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glStencilFunc, void)
    {
        ::glStencilFunc(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glStencilMask, void)
    {
        ::glStencilMask(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLModule::glStencilOp, void)
    {
        ::glStencilOp(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoord1f, void)
    {
        ::glTexCoord1f(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoord2f, void)
    {
        ::glTexCoord2f(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoord2fv, void)
    {
        const Vector2f& v = NODE_ARG(0, Vector2f);
        ::glTexCoord2f(v[0], v[1]);
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoord3f, void)
    {
        ::glTexCoord3f(NODE_ARG(0, float), NODE_ARG(1, float),
                       NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoord3fv, void)
    {
        const Vector3f& v = NODE_ARG(0, Vector3f);
        ::glTexCoord3f(v[0], v[1], v[2]);
    }

    NODE_IMPLEMENTATION(GLModule::glTexEnvf, void)
    {
        ::glTexEnvf(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexEnvi, void)
    {
        ::glTexEnvi(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glTexGenf, void)
    {
        ::glTexGenf(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexGeni, void)
    {
        ::glTexGeni(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glTexImage1D, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int target = NODE_ARG(0, int);
        int level = NODE_ARG(1, int);
        int internalFormat = NODE_ARG(2, int);
        int width = NODE_ARG(3, int);
        int border = NODE_ARG(4, int);
        int format = NODE_ARG(5, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(6, DynamicArray);
        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType() || t == c->vec3fType() || t == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        ::glTexImage1D(target, level, internalFormat, width, border, format,
                       type, pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexImage2D, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int target = NODE_ARG(0, int);
        int level = NODE_ARG(1, int);
        int internalFormat = NODE_ARG(2, int);
        int width = NODE_ARG(3, int);
        int height = NODE_ARG(4, int);
        int border = NODE_ARG(5, int);
        int format = NODE_ARG(6, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(7, DynamicArray);
        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType() || t == c->vec3fType() || t == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        ::glTexImage2D(target, level, internalFormat, width, height, border,
                       format, type, pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexParameterf, void)
    {
        ::glTexParameterf(NODE_ARG(0, int), NODE_ARG(1, int),
                          NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glTexParameteri, void)
    {
        ::glTexParameteri(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(GLModule::glTexSubImage1D, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int target = NODE_ARG(0, int);
        int level = NODE_ARG(1, int);
        int xoffset = NODE_ARG(2, int);
        int width = NODE_ARG(3, int);
        int format = NODE_ARG(4, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(5, DynamicArray);
        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType() || t == c->vec3fType() || t == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        ::glTexSubImage1D(target, level, xoffset, width, format, type,
                          pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexSubImage2D, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int target = NODE_ARG(0, int);
        int level = NODE_ARG(1, int);
        int xoffset = NODE_ARG(2, int);
        int yoffset = NODE_ARG(3, int);
        int width = NODE_ARG(4, int);
        int height = NODE_ARG(5, int);
        int format = NODE_ARG(6, int);

        DynamicArray* pixelArray = NODE_ARG_OBJECT(7, DynamicArray);
        const Type* t = pixelArray->elementType();

        GLenum type = 0;
        if (t == c->floatType() || t == c->vec3fType() || t == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (t == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        ::glTexSubImage2D(target, level, xoffset, yoffset, width, height,
                          format, type, pixelArray->data<void>());
    }

    NODE_IMPLEMENTATION(GLModule::glTranslatef, void)
    {
        ::glTranslatef(NODE_ARG(0, float), NODE_ARG(1, float),
                       NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glVertex2f, void)
    {
        ::glVertex2f(NODE_ARG(0, float), NODE_ARG(1, float));
    }

    NODE_IMPLEMENTATION(GLModule::glVertex2fv, void)
    {
        Vector2f v = NODE_ARG(0, Vector2f);
        ::glVertex2f(v[0], v[1]);
    }

    NODE_IMPLEMENTATION(GLModule::glVertex3f, void)
    {
        ::glVertex3f(NODE_ARG(0, float), NODE_ARG(1, float),
                     NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(GLModule::glVertex3fv, void)
    {
        const Vector3f& v = NODE_ARG(0, Vector3f);
        ::glVertex3f(v[0], v[1], v[2]);
    }

    NODE_IMPLEMENTATION(GLModule::glViewport, void)
    {
        ::glViewport(NODE_ARG(0, int), NODE_ARG(1, int), NODE_ARG(2, int),
                     NODE_ARG(3, int));
    }

    NODE_IMPLEMENTATION(GLModule::glVertexPointer4fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glVertexPointer(4, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glVertexPointer3fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glVertexPointer(3, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glVertexPointer2fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glVertexPointer(2, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glNormalPointer3fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glNormalPointer(GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glColorPointer4fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glColorPointer(4, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glColorPointer3fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glColorPointer(3, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glIndexPointer, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glIndexPointer(GL_INT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoordPointer4fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glTexCoordPointer(4, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoordPointer3fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glTexCoordPointer(3, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoordPointer2fv, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glTexCoordPointer(2, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glTexCoordPointer1f, void)
    {
        DynamicArray* varray = NODE_ARG_OBJECT(0, DynamicArray);
        ::glTexCoordPointer(1, GL_FLOAT, 0, varray->data<GLvoid>());
    }

    NODE_IMPLEMENTATION(GLModule::glInterleavedArrays, void)
    {
        DynamicArray* array = NODE_ARG_OBJECT(2, DynamicArray);

        ::glInterleavedArrays(NODE_ARG(0, int), NODE_ARG(1, int),
                              array->data<GLvoid>());
    }

} // namespace Mu

#ifdef _MSC_VER
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif
