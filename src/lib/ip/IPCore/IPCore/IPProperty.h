//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__IPProperty__h__
#define __IPCore__IPProperty__h__
#include <iostream>
#include <TwkContainer/Property.h>

namespace IPCore
{

    //
    //  This can be attached to a property (usually when declared) to give
    //  the application more information about how a property should be
    //  used.
    //
    //  persistent  means the property should be stored in the file
    //  animatable  is self evident
    //  graphEdit   means changing the property causes a graph topology
    //              change
    //
    //  initialRefCount should be set to 1 if the PropertyInfo is going to
    //                  be shared
    //

    class PropertyInfo : public TwkContainer::Property::Info
    {
    public:
        //
        //  Types
        //

        enum PropertyFlags
        {
            NotPersistent = 0,
            Persistent = 1 << 0,
            NotCopyable = 1 << 1,
            Animatable = 1 << 2,
            RequiresGraphEdit = 1 << 3,
            OutputOnly = 1 << 4,
            RequiresProgramFlush = 1 << 5,
            RequiresAudioFlush = 1 << 6,
            ExcludeFromProfile = 1 << 7
        };

        typedef unsigned int Flags;

        PropertyInfo(Flags flags, size_t initialRefCount = 0,
                     const std::string& interp = "")
            : TwkContainer::Property::Info((flags & Persistent) != 0,
                                           (flags & NotCopyable) == 0,
                                           initialRefCount, interp)
            , m_flags(flags)
        {
        }

        bool requiresGraphEdit() const { return m_flags & RequiresGraphEdit; }

        bool animatable() const { return m_flags & Animatable; }

        bool outputOnly() const { return m_flags & OutputOnly; }

        bool requiresProgramFlush() const
        {
            return m_flags & RequiresProgramFlush;
        }

        bool requiresAudioFlush() const { return m_flags & RequiresAudioFlush; }

        bool excludedFromProfile() const
        {
            return m_flags & ExcludeFromProfile;
        }

    protected:
        ~PropertyInfo() {}

    private:
        Flags m_flags;
    };

} // namespace IPCore

#endif // __IPCore__IPProperty__h__
