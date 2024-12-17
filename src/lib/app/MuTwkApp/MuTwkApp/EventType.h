//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__EventType__h__
#define __TwkApp__EventType__h__
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <Mu/Vector.h>

namespace TwkApp
{
    class Document;
    class Event;

    class EventType : public Mu::Class
    {
    public:
        EventType(Mu::Context*, Mu::Class* super = 0);
        ~EventType();

        class EventInstance : public Mu::ClassInstance
        {
        public:
            EventInstance(const Mu::Class*);

            const Event* event;
            const Document* document;

        protected:
            friend class EventType;
        };

        //
        //	API
        //

        virtual Mu::Object* newObject() const;
        virtual void deleteObject(Mu::Object*) const;
        virtual void outputValue(std::ostream&, const Mu::Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const Mu::ValuePointer,
                                          Mu::Type::ValueOutputState&) const;
        virtual void load();

        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(pointer, Mu::Vector2f);
        static NODE_DECLARATION(relativePointer, Mu::Vector2f);
        static NODE_DECLARATION(reference, Mu::Vector2f);
        static NODE_DECLARATION(domainVerticalFlip, bool);
        static NODE_DECLARATION(domain, Mu::Vector2f);
        static NODE_DECLARATION(subDomain, Mu::Vector2f);
        static NODE_DECLARATION(buttons, int);
        static NODE_DECLARATION(modifiers, int);
        static NODE_DECLARATION(name, Mu::Pointer);
        static NODE_DECLARATION(key, int);
        static NODE_DECLARATION(dataContents, Mu::Pointer);
        static NODE_DECLARATION(interp, Mu::Pointer);
        static NODE_DECLARATION(contents, Mu::Pointer);
        static NODE_DECLARATION(contentsArray, Mu::Pointer);
        static NODE_DECLARATION(returnContents, Mu::Pointer);
        static NODE_DECLARATION(contentType, int);
        static NODE_DECLARATION(contentMimeType, Mu::Pointer);
        static NODE_DECLARATION(timeStamp, float);
        static NODE_DECLARATION(reject, void);
        static NODE_DECLARATION(setReturnContent, void);
        static NODE_DECLARATION(sender, Mu::Pointer);
        static NODE_DECLARATION(pressure, float);
        static NODE_DECLARATION(tangentialPressure, float);
        static NODE_DECLARATION(rotation, float);
        static NODE_DECLARATION(xTilt, int);
        static NODE_DECLARATION(yTilt, int);
        static NODE_DECLARATION(activationTime, float);

    private:
    };

} // namespace TwkApp

#endif // __TwkApp__EventType__h__
