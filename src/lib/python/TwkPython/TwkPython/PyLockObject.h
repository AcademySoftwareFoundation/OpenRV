//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#pragma once

#include <Python.h>

class PyLockObject
{
 public:
  //
  //  Find PyThreadState of current thread.  If this thread does
  //  not have the GIL, acquire it and swap in this state.
  //
  PyLockObject();

  //
  //  If we acquired the GIL in the constructor, release it here.
  //  If we already had the GIL when the constructor ran, we leave
  //  that state unchanged.
  //
  ~PyLockObject();

 private:
  //
  //  True IFF we acquired the GIL in the constructor and must
  //  release it in the destructor.
  //
  PyGILState_STATE _gilState;
};