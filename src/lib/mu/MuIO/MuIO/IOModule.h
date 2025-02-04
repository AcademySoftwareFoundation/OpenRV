#ifndef __runtime__IOModule__h__
#define __runtime__IOModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{

    class IOModule : public Module
    {
    public:
        IOModule(Context* c, const char* name);
        virtual ~IOModule();

        virtual void load();

        //
        //  Print Functions
        //

        static NODE_IMPLEMENTATION(printString, Pointer);
        static NODE_IMPLEMENTATION(print_int, Pointer);
        static NODE_IMPLEMENTATION(print_float, Pointer);
        static NODE_IMPLEMENTATION(print_double, Pointer);
        static NODE_IMPLEMENTATION(print_bool, Pointer);
        static NODE_IMPLEMENTATION(print_byte, Pointer);
        static NODE_IMPLEMENTATION(print_char, Pointer);
        static NODE_IMPLEMENTATION(print_endl, Pointer);
        static NODE_IMPLEMENTATION(print_flush, Pointer);

        static NODE_IMPLEMENTATION(readString, Pointer);
        static NODE_IMPLEMENTATION(read_int, int);
        static NODE_IMPLEMENTATION(read_float, float);
        static NODE_IMPLEMENTATION(read_double, double);
        static NODE_IMPLEMENTATION(read_bool, bool);
        static NODE_IMPLEMENTATION(read_byte, char);
        static NODE_IMPLEMENTATION(read_char, char);
        static NODE_IMPLEMENTATION(read_line, Pointer);
        static NODE_IMPLEMENTATION(read_all, Pointer);
        static NODE_IMPLEMENTATION(read_all_bytes, Pointer);

        static NODE_IMPLEMENTATION(in, Pointer);
        static NODE_IMPLEMENTATION(out, Pointer);
        static NODE_IMPLEMENTATION(err, Pointer);

        static NODE_IMPLEMENTATION(serialize, int);
        static NODE_IMPLEMENTATION(deserialize, Pointer);

        static NODE_IMPLEMENTATION(directory, Pointer);

        static NODE_IMPLEMENTATION(basename, Pointer);
        static NODE_IMPLEMENTATION(extension, Pointer);
        static NODE_IMPLEMENTATION(without_extension, Pointer);
        static NODE_IMPLEMENTATION(dirname, Pointer);
        static NODE_IMPLEMENTATION(exists, bool);
        static NODE_IMPLEMENTATION(expand, Pointer);
        static NODE_IMPLEMENTATION(join, Pointer);
        static NODE_IMPLEMENTATION(concat_paths, Pointer);
        static NODE_IMPLEMENTATION(path_separator, Pointer);
        static NODE_IMPLEMENTATION(concat_separator, Pointer);
    };

} // namespace Mu

#endif // __runtime__IOModule__h__
