//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __IPGraph__OCIOIPNode__h__
#define __IPGraph__OCIOIPNode__h__
#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore {

/// Provides 3D and Channel OCIOs for DisplayIPNode and ColorIPNode

///
/// OCIOIPNode -- use OpenColorIO transform
///

struct OCIOState;

class OCIOIPNode : public IPNode
{
  public:
    //
    //  Constructors
    //

    OCIOIPNode(const std::string& name, 
               const NodeDefinition* def,
               IPGraph* graph, 
               GroupIPNode* group=0);

    virtual ~OCIOIPNode();
    virtual IPImage* evaluate(const Context& context);
    std::string stringProp(const std::string&, const std::string&) const;
    int intProp(const std::string&, int) const;

    virtual void readCompleted(const std::string&, unsigned int);
    virtual void propertyChanged(const Property*);

    void updateConfig();

  protected:
    void updateContext();
    
  private:
    IntProperty*    m_activeProperty;
    IntProperty*    m_lutSize;
    StringProperty* m_configDescription;
    StringProperty* m_configWorkingDir;
    FrameBuffer*    m_lutfb;
    std::string     m_lutSamplerName;
    FrameBuffer*    m_prelutfb;
    OCIOState*      m_state;
    pthread_mutex_t m_lock;
};

} // Rv

#endif // __IPGraph__OCIOIPNode__h__
