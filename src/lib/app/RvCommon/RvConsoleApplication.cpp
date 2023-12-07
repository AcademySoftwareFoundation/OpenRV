//******************************************************************************
// Copyright (c) 2023 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvCommon/RvConsoleApplication.h>

namespace Rv
{

  RvConsoleApplication::RvConsoleApplication()
      : IPCore::Application(), m_pyLibrary( INTERNAL_APPLICATION_NAME )
  {
    m_pyLibrary.setName( "py-media-library" );
  }

}  // namespace Rv