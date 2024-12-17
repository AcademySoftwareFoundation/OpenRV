//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
static Pointer codecForName_QTextCodec_string(Thread& NODE_THREAD, Pointer p)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    StringType::String* str = reinterpret_cast<StringType::String*>(p);
    return makeqpointer<QTextCodecType>(
        c, QTextCodec::codecForName(str->c_str()), "qt.QTextCodec");
}

static NODE_IMPLEMENTATION(codecForName2, Pointer)
{
    NODE_RETURN(
        codecForName_QTextCodec_string(NODE_THREAD, NODE_ARG(0, Pointer)));
}
