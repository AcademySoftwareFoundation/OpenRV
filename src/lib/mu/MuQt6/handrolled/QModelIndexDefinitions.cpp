//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static NODE_IMPLEMENTATION(_n_internalPointer, Pointer)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QModelIndex arg0 = getqtype<QModelIndexType>(NODE_ARG(0, Pointer));
    NODE_RETURN(Pointer(arg0.internalPointer()));
}
