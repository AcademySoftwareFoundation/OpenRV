//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <OCIONodes/OCIOLUT.h>
#include <TwkExc/Exception.h>

namespace IPCore
{

  OCIOLUT::~OCIOLUT()
  {
    if( m_lutfb )
    {
      if( !m_lutfb->hasStaticRef() || m_lutfb->staticUnRef() )
      {
        delete m_lutfb;
      }
      m_lutfb = 0;
    }
  }

}  // namespace IPCore
