//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext_config__h__
#define __stl_ext_config__h__

#ifndef NDEBUG
#define STL_EXT_DEBUG true
#endif

#ifdef WIN32
#if defined(STL_EXT_BUILD)
#define STL_EXT_EXPORT __declspec(dllexport)
#else
#define STL_EXT_EXPORT __declspec(dllimport)
#endif
#else
#define STL_EXT_EXPORT
#endif

namespace stl_ext
{

} // namespace stl_ext

#endif // __stl_ext_config__h__
