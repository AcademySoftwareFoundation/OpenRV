//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <TwkFB/FrameBuffer.h>

#include <string>

namespace IPCore
{

  // An OCIO LUT base representation (a GPU texture) with its associated frame
  // buffer.
  //
  class OCIOLUT
  {
   public:
    // Returns the LUT sampler name
    std::string samplerName() const
    {
      return m_samplerName;
    };

    // Returns the LUT sampler type: sampler3D, sampler2D, or sampler1D
    virtual std::string samplerType() const = 0;

    // Returns the FrameBuffer associated with the LUT containing the LUT values
    TwkFB::FrameBuffer* lutfb() const
    {
      return m_lutfb;
    }

   protected:
    OCIOLUT() {};
    virtual ~OCIOLUT();

    TwkFB::FrameBuffer* m_lutfb{ nullptr };
    std::string m_samplerName;
  };

}  // namespace IPCore
