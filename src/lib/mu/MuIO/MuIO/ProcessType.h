#ifndef __MuIO__ProcessType__h__
#define __MuIO__ProcessType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <Mu/Thread.h>
#include <MuIO/OStreamType.h>
#include <MuIO/IStreamType.h>
#include <Mu/Vector.h>
#include <iosfwd>
#include <string>
#include <MuIO/exec-stream.h>

namespace Mu
{

    class ProcessType : public Class
    {
    public:
        ProcessType(Context* c, const char* name, Class* super = 0);
        virtual ~ProcessType();

        class PipeStream : public ClassInstance
        {
        public:
            PipeStream(const Class*);
            ~PipeStream();

            exec_stream_t* exec_stream;
            OStreamType::OStream* in;
            IStreamType::IStream* out;
            IStreamType::IStream* err;
        };

        //
        //	Return a new Object
        //

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;

        //
        // Output the appropriate Value in human readable form (note: you
        // can call the static function if you already have a std::string.
        //

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(toBool, bool);
        static NODE_DECLARATION(inStream, Pointer);
        static NODE_DECLARATION(outStream, Pointer);
        static NODE_DECLARATION(errStream, Pointer);
        static NODE_DECLARATION(close, bool);
        static NODE_DECLARATION(close_in, bool);
        static NODE_DECLARATION(kill, void);
        static NODE_DECLARATION(exit_code, int);
    };

} // namespace Mu

#endif // __MuIO__ProcessType__h__
