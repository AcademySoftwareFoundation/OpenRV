//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static char** global_argv = 0;

static NODE_IMPLEMENTATION(_n_QApplication0, Pointer)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* inst = NODE_ARG_OBJECT(0, ClassInstance);
    DynamicArray* args = NODE_ARG_OBJECT(1, DynamicArray);

    int n = args->size();
    global_argv = new char*[n + 1];
    global_argv[n] = 0;

    for (size_t i = 0; i < n; i++)
    {
        Mu::UTF8String utf8 = args->element<StringType::String*>(i)->utf8();
        global_argv[i] = strdup(utf8.c_str());
    }

    setobject(inst, new QApplication(n, global_argv));
    NODE_RETURN(inst);
}
