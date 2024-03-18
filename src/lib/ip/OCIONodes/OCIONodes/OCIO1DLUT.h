//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <OCIONodes/OCIOLUT.h>

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include <vector>

namespace IPCore
{

  namespace OCIO = OCIO_NAMESPACE;

  // An OCIO 1D LUT representation (a GPU texture) with its associated
  // frame buffer.
  //
  class OCIO1DLUT : public OCIOLUT
  {
   public:
    OCIO1DLUT( OCIO::GpuShaderDescRcPtr& shaderDesc, unsigned int idx,
               const std::string& shaderCacheID );
    virtual ~OCIO1DLUT() {};

    // Returns the LUT sampler type: sampler3D, sampler2D, or sampler1D
    // Note: a 2D texture is needed to hold large LUTs.
    std::string samplerType() const override
    {
      return m_height == 1 ? "sampler1D" : "sampler2D";
    }

   private:
    int m_height{ 1 };
  };

}  // namespace IPCore
