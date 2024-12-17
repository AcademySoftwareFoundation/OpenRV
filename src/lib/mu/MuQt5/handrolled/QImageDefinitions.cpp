//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static Pointer QImage_QImage_QImage_QImage_string_string(Thread& NODE_THREAD,
                                                         Pointer param_this,
                                                         Pointer param_fileName,
                                                         Pointer param_format)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QString arg1 = qstring(param_fileName);
    const char* arg2 =
        param_format
            ? reinterpret_cast<StringType::String*>(param_format)->c_str()
            : 0;
    setqtype<QImageType>(param_this, QImage(arg1, arg2));
    return param_this;
}

static NODE_IMPLEMENTATION(_n_QImage8, Pointer)
{
    return QImage_QImage_QImage_QImage_string_string(
        NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
        NODE_ARG(2, Pointer));
}
