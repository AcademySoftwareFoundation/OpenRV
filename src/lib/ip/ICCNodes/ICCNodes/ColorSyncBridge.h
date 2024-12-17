//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __ICCNodes__ColorSyncBridge__h__
#define __ICCNodes__ColorSyncBridge__h__
#include <iostream>

namespace IPCore
{

    //
    //  For some reason we can't include Foundation.h in a .cpp file --
    //  you have to make it a .mm. This file hides all of the
    //  CoreFoundation API so we can call it cleanly
    //

    typedef void* CSProfile;
    typedef void* CSTransform;

#ifdef USE_COLORSYNC
    CSProfile newColorSyncProfileFromData(void*, size_t);
    CSProfile newColorSyncProfileFromPath(const std::string&);
    void deleteColorSyncProfile(CSProfile);
    CSTransform newColorSyncTransformFromProfiles(CSProfile, CSProfile);
    void deleteColorSyncTransform(CSTransform);
    void convert(CSTransform, size_t, size_t, float*, float*);
#else
    inline CSProfile newColorSyncProfileFromData(void*, size_t) { return 0; }

    inline CSProfile newColorSyncProfileFromPath(const std::string&)
    {
        return 0;
    }

    inline void deleteColorSyncProfile(CSProfile) {}

    inline CSTransform newColorSyncTransformFromProfiles(CSProfile, CSProfile)
    {
        return 0;
    }

    inline void deleteColorSyncTransform(CSTransform) {}

    inline void convert(CSTransform, size_t, size_t, float*, float*) {}
#endif

} // namespace IPCore

#endif // __ICCNodes__ColorSyncBridge__h__
