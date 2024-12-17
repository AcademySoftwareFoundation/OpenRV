//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/CombineIPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderUtil.h>
#include <TwkMath/Function.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    CombineIPInstanceNode::CombineIPInstanceNode(const std::string& name,
                                                 const NodeDefinition* def,
                                                 IPGraph* g, GroupIPNode* group)
        : IPInstanceNode(name, def, g, group)
        , m_structureInfoDirty(true)
    {
        setMaxInputs(1);
        setHasLinearTransform(true); // fit to aspect
        pthread_mutex_init(&m_lock, 0);

        m_autoSize = declareProperty<IntProperty>("output.autoSize", 1);
        m_outputSize = createProperty<IntProperty>("output.size");
        m_outputSize->resize(2);
        m_outputSize->front() = 1920;
        m_outputSize->back() = 1080;

        findProps();
    }

    CombineIPInstanceNode::~CombineIPInstanceNode()
    {
        pthread_mutex_destroy(&m_lock);
    }

    bool CombineIPInstanceNode::testInputs(const IPNodes& inputs,
                                           std::ostringstream& msg) const
    {
        if (inputs.size() != 1)
        {
            string un = uiName();
            string n = name();

            if (un == n)
                msg << n;
            else
                msg << un << " (" << n << ")";
            msg << " requires exactly 1 input" << endl;
            return false;
        }
        else
        {
            return IPInstanceNode::testInputs(inputs, msg);
        }
    }

    void CombineIPInstanceNode::findProps()
    {
        const Shader::Function* F = definition()->function();

        m_offsetProps.clear();
        m_frameProps.clear();
        m_layerProps.clear();
        m_viewProps.clear();
        m_channelProps.clear();
        m_eyeProps.clear();

        for (size_t i = 0; i < F->imageParameters().size(); i++)
        {
            ostringstream offsetName;
            ostringstream frameName;
            ostringstream layerName;
            ostringstream viewName;
            ostringstream channelName;
            ostringstream eyeName;

            offsetName << "parameters.offset" << i;
            frameName << "parameters.frame" << i;
            layerName << "parameters.layer" << i;
            viewName << "parameters.view" << i;
            channelName << "parameters.channel" << i;
            eyeName << "parameters.eye" << i;

            m_offsetProps.push_back(property<IntProperty>(offsetName.str()));
            m_frameProps.push_back(property<IntProperty>(frameName.str()));
            m_layerProps.push_back(property<StringProperty>(layerName.str()));
            m_viewProps.push_back(property<StringProperty>(viewName.str()));
            m_channelProps.push_back(
                property<StringProperty>(channelName.str()));
            m_eyeProps.push_back(property<IntProperty>(eyeName.str()));
        }
    }

    void CombineIPInstanceNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_autoSize || p == m_outputSize)
            {
                lock();
                m_structureInfoDirty = true;
                unlock();
                propagateImageStructureChange();
                // computeRanges();
            }
        }

        IPInstanceNode::propertyChanged(p);
    }

    void CombineIPInstanceNode::computeStructure()
    {
        if (isDeleting())
            return;

        //
        //  NOTE: this function computes the range for this node based on
        //  the first input as the start. At the end we compute an offset
        //  from m_info so that the result range is actually 1 to whatever
        //

        m_rangeInfo = inputs()[0]->imageRangeInfo();
        m_structureInfo = inputs()[0]->imageStructureInfo(
            graph()->contextForFrame(m_rangeInfo.start));
        const float pa = m_structureInfo.pixelAspect;

        const float wpa = std::max(1.0f, pa);
        const float hpa = std::max(1.0f, 1.0f / pa);

        if (m_autoSize->front())
        {
            m_outputSize->front() = m_structureInfo.width * wpa;
            m_outputSize->back() = m_structureInfo.height * hpa;
        }
        else
        {
            m_structureInfo.width = m_outputSize->front();
            m_structureInfo.height = m_outputSize->back();
        }
    }

    void CombineIPInstanceNode::propertyDeleted(const std::string&)
    {
        findProps();
    }

    void CombineIPInstanceNode::newPropertyCreated(const Property*)
    {
        findProps();
    }

    void
    CombineIPInstanceNode::inputImageStructureChanged(int inputIndex,
                                                      PropagateTarget target)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();
    }

    IPNode::ImageStructureInfo
    CombineIPInstanceNode::imageStructureInfo(const Context& context) const
    {
        if (m_structureInfoDirty)
            lazyUpdateRanges();
        return m_structureInfo;
    }

    void CombineIPInstanceNode::lazyUpdateRanges() const
    {
        lock();

        if (m_structureInfoDirty)
        {
            try
            {
                const_cast<CombineIPInstanceNode*>(this)->computeStructure();
            }
            catch (...)
            {
                unlock();
                throw;
            }
        }

        unlock();
    }

    void CombineIPInstanceNode::contextAtInput(Context& context, size_t i)
    {
        if (IntProperty* p = m_frameProps[i])
            context.frame = p->front();
        if (IntProperty* p = m_offsetProps[i])
            context.frame += p->front();
        if (IntProperty* p = m_eyeProps[i])
        {
            context.eye = p->front();
            context.stereo = true;
        }

        //
        //  NOTE the order here matters. Views will override layers will
        //  override channels.
        //

        if (StringProperty* p = m_viewProps[i])
        {
            context.component.type = ViewComponent;
            context.component.name.resize(1);
            context.component.name.front() = p->front();
        }

        if (StringProperty* p = m_layerProps[i])
        {
            context.component.type = LayerComponent;
            context.component.name.resize(2);
            context.component.name.back() = p->front();
        }

        if (StringProperty* p = m_channelProps[i])
        {
            context.component.type = ViewComponent;
        }
    }

    IPImage* CombineIPInstanceNode::evaluate(const Context& context)
    {
        const Shader::Function* F = definition()->function();
        if (!isActive())
            return IPInstanceNode::evaluate(context);

        if (m_structureInfoDirty)
            lazyUpdateRanges();

        const int width = m_structureInfo.width;
        const int height = m_structureInfo.height;
        const float aspect = float(width) / float(height);

        IPImage* root =
            new IPImage(this, IPImage::MergeRenderType, width, height, 1.0,
                        IPImage::IntermediateBuffer, IPImage::FloatDataType);
        IPNodes nodes = inputs();
        const int ninputs = nodes.size();
        if (ninputs == 0)
            return root;

        vector<Shader::Expression*> inExpressions;
        IPImageVector images;
        IPImageSet modifiedImages;

        for (size_t i = 0; i < F->imageParameters().size(); i++)
        {
            Context c = context;
            contextAtInput(c, i);

            try
            {
                IPImage* current = nodes[0]->evaluate(c);

                if (!current)
                {
                    TWK_THROW_STREAM(
                        EvaluationFailedExc,
                        "CombineIPInstanceNode evaluation failed on node "
                            << nodes[0]->name());
                }

                current->fitToAspect(aspect);
                images.push_back(current);
            }
            catch (std::exception&)
            {
                ThreadType thread = context.thread;

                TWK_CACHE_LOCK(context.cache, "thread=" << thread);
                context.cache.checkInAndDelete(images);
                TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
                delete root;
                throw;
            }
        }

        convertBlendRenderTypeToIntermediate(images, modifiedImages);

        balanceResourceUsage(F->isFilter() ? IPNode::filterAccumulate
                                           : IPNode::accumulate,
                             images, modifiedImages, 8, 8, 81);

        assembleMergeExpressions(root, images, modifiedImages, F->isFilter(),
                                 inExpressions);

        root->appendChildren(images);
        root->mergeExpr =
            bind(root, inExpressions, context); // IPInstanceNode::bind
        root->shaderExpr = Shader::newSourceRGBA(root);
        root->recordResourceUsage();

        return root;
    }

    IPImageID* CombineIPInstanceNode::evaluateIdentifier(const Context& context)
    {
        const Shader::Function* F = definition()->function();

        if (m_structureInfoDirty)
            lazyUpdateRanges();
        const int width = m_structureInfo.width;
        const int height = m_structureInfo.height;
        const float aspect = float(width) / float(height);

        IPNodes nodes = inputs();
        IPImageID* imageID = new IPImageID();

        for (size_t i = 0; i < F->imageParameters().size(); i++)
        {
            Context c = context;
            contextAtInput(c, i);

            if (IPImageID* id = nodes[0]->evaluateIdentifier(c))
            {
                imageID->appendChild(id);
            }
        }

        return imageID;
    }

    void CombineIPInstanceNode::metaEvaluate(const Context& context,
                                             MetaEvalVisitor& visitor)
    {
        const Shader::Function* F = definition()->function();

        if (m_structureInfoDirty)
            lazyUpdateRanges();

        IPNodes nodes = inputs();

        visitor.enter(context, this);

        for (size_t i = 0; i < F->imageParameters().size(); i++)
        {
            Context c = context;
            contextAtInput(c, i);

            if (visitor.traverseChild(c, 0, this, nodes[0]))
            {
                nodes[0]->metaEvaluate(c, visitor);
            }
        }

        visitor.leave(context, this);
    }

    void CombineIPInstanceNode::readCompleted(const string& t, unsigned int v)
    {
        lock();
        m_structureInfoDirty = true;
        unlock();

        IPInstanceNode::readCompleted(t, v);
    }

} // namespace IPCore
