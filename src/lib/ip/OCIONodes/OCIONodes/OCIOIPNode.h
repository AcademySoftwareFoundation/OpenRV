//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#pragma once

#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

#include <QMutex>
#include <OpenColorIO/OpenColorIO.h>

namespace IPCore {

/// Provides 3D and Channel OCIOs for DisplayIPNode and ColorIPNode

///
/// OCIOIPNode -- use OpenColorIO transform
///

struct OCIOState;

namespace OCIO = OCIO_NAMESPACE;

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
    bool useRawConfig() const {return m_useRawConfig;}

  protected:
    void updateContext();

    OCIO::MatrixTransformRcPtr createMatrixTransformXYZToRec709() const;
    OCIO::MatrixTransformRcPtr getMatrixTransformXYZToRec709();
    OCIO::MatrixTransformRcPtr getMatrixTransformRec709ToXYZ();

  private:

    IntProperty*    m_activeProperty{nullptr};
    IntProperty*    m_lutSize{nullptr};
    StringProperty* m_configDescription{nullptr};
    StringProperty* m_configWorkingDir{nullptr};
    FrameBuffer*    m_lutfb{nullptr};
    std::string     m_lutSamplerName;
    FrameBuffer*    m_prelutfb{nullptr};
    OCIOState*      m_state{nullptr};
    mutable QMutex  m_lock;
    bool            m_useRawConfig{false};

    // synlinearize/syndisplay functions
    StringProperty* m_inTransformURL{nullptr};
    ByteProperty*   m_inTransformData{nullptr};
    StringProperty* m_outTransformURL{nullptr};
    OCIO::GroupTransformRcPtr m_transform;
    OCIO::MatrixTransformRcPtr m_matrix_xyz_to_rec709;
    OCIO::MatrixTransformRcPtr m_matrix_rec709_to_xyz;
};

} // IPCore
