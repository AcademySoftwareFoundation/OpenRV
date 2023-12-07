//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#pragma once

#include <PyMediaLibrary/PyNode.h>
#include <PyMediaLibrary/PyRootNode.h>

#include <TwkMediaLibrary/Library.h>

#include <Python.h>

namespace TwkMediaLibrary
{
  using Plugin = PyObject*;

  class PyMediaLibrary;

  class PyMediaNode : public PyNode, public MediaAPI
  {
   public:
    PyMediaNode( Library* lib, URL url, PyNode* parent, Plugin plugin,
                 std::string pluginName );
    virtual ~PyMediaNode();
    virtual const Node* node() const;
    URL mediaURL() const override;
    bool isStreaming() const override;
    bool isRedirecting() const override;
    const Node* baseMediaNode() const override;
    HTTPCookieVector httpCookies() const override;
    HTTPHeaderVector httpHeaders() const override;
    URL httpRedirection() const override;

   private:
    const URL m_url;
    PyObject* m_plugin;
  };
}  // namespace TwkMediaLibrary