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

  // An OCIO 3D LUT representation (a GPU texture) with its associated
  // frame buffer.
  //
  class OCIO3DLUT : public OCIOLUT
  {
   public:
    OCIO3DLUT( OCIO::GpuShaderDescRcPtr& shaderDesc, unsigned int idx,
               const std::string& shaderCacheID );
    virtual ~OCIO3DLUT() {};

    // Returns the LUT sampler type: sampler3D, sampler2D, or sampler1D
    std::string samplerType() const override
    {
      return "sampler3D";
    }
  };

}  // namespace IPCore
