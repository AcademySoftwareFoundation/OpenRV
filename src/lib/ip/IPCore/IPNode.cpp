//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/AdaptorIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/IPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderUtil.h>
#include <algorithm>
#include <TwkMath/Frustum.h>
#include <TwkUtil/sgcHop.h>
#include <stl_ext/stl_ext_algo.h>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace std;
    using namespace stl_ext;
    using namespace TwkMath;

    IPNode::IPNode(const string& name, const NodeDefinition* definition,
                   IPGraph* graph, GroupIPNode* group)
        : PropertyContainer()
        , m_definition(definition)
        , m_graph(graph)
        , m_group(group)
        , m_maxInputs(-1)
        , m_minInputs(1)
        , m_deleting(false)
        , m_writable(true)
        , m_unconstrainedInputs(false)
        , m_metaSearchable(true)
        , m_hasAudio(false)
        , m_hasVideo(false)
        , m_undoRefCount(0)
    {
        setName(graph->uniqueName(name));
        setProtocol(definition->stringValue("node.name"));
        setProtocolVersion(definition->intValue("node.version"));

        graph->addNode(this);
        if (group)
            group->add(this);
    }

    IPNode::~IPNode()
    {
        while (!m_outputs.empty())
        {
            m_outputs.front()->removeInput(this);
        }

        assert(m_outputs.empty());

        deleteInputs();
        if (group())
            group()->remove(this);
        if (graph())
            graph()->removeNode(this);
    }

    void IPNode::copyNode(const IPNode* node)
    {
        if (node->definition() == definition())
        {
            copy(node); // property container copy
        }
    }

    void IPNode::copy(const PropertyContainer* p)
    {
        PropertyContainer::copy(p);
    }

    bool IPNode::isWritable() const
    {
        if (group())
        {
            if (!group()->isWritable())
                return false;
        }

        return m_writable;
    }

    void IPNode::deleteInputs()
    {
        while (!m_inputs.empty())
        {
            IPNode* n = m_inputs.front();
            n->willDelete();
            removeInput(n);
            delete n;
        }
    }

    int IPNode::indexOfChild(IPNode* n)
    {
        IPNodes::iterator it = std::find(m_inputs.begin(), m_inputs.end(), n);

        if (it == m_inputs.end())
            return -1;
        return int(it - m_inputs.begin());
    }

    void IPNode::appendInput(IPNode* n)
    {
        IPNodes newInputs(m_inputs.size() + 1);

        if (!m_inputs.empty())
        {
            std::copy(m_inputs.begin(), m_inputs.end(), newInputs.begin());
        }

        newInputs.back() = n;
        setInputs(newInputs);
    }

    void IPNode::insertInput(IPNode* n, size_t index)
    {
        IPNodes newInputs = m_inputs;
        newInputs.insert(newInputs.begin() + index, n);
        setInputs(newInputs);
    }

    bool IPNode::isInput(IPNode* n) const
    {
        IPNodes::const_iterator it =
            std::find(m_inputs.begin(), m_inputs.end(), n);

        return it != m_inputs.end();
    }

    void IPNode::removeInput(IPNode* n)
    {
        if (!isInput(n))
        {
            cout << "ERROR: asked to remove non-existant input " << n->name()
                 << " from " << name() << endl;

            return;
        }

        if (m_inputs.size())
        {
            IPNodes newInputs(m_inputs.size() - 1);

            if (!newInputs.empty())
            {
                for (size_t i = 0, off = 0; i < m_inputs.size(); i++)
                {
                    if (off == 0 && m_inputs[i] == n) // might be two
                    {
                        off = 1;
                    }
                    else
                    {
                        assert((i - off) < newInputs.size());
                        newInputs[i - off] = m_inputs[i];
                    }
                }
            }

            setInputs(newInputs);
        }
    }

    void IPNode::setGroup(GroupIPNode* g)
    {
        m_group = g;
        m_group->add(this);
    }

    const string IPNode::uiName() const
    {
        if (const StringProperty* sp = property<StringProperty>("ui.name"))
        {
            return sp->front();
        }
        else
        {
            return name();
        }
    }

    void IPNode::disconnectInputs()
    {
        IPNodes empty;
        setInputs(empty);
    }

    void IPNode::disconnectOutputs()
    {
        while (!m_outputs.empty())
            m_outputs.back()->removeInput(this);
    }

    void IPNode::disconnectInputsAtomic()
    {
        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->outputDisconnect(this);
        }

        m_inputs.clear();
    }

    void IPNode::setInputs(const IPNodes& nodes)
    {
        HOP_PROF_FUNC();
        if (m_inputs == nodes)
            return;

        //
        //  Sanity check
        //

        const int num = maximumNumberOfInputs();

        if (num >= 0 && num < nodes.size())
        {
            //
            //  Throw, too many inputs for this node type
            //

            throw SingleInputOnlyExc();
        }

        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (!nodes[i])
            {
                cout << "ERROR: NULL node passed to setInputs in node "
                     << name() << endl;
            }
        }

        //
        //  Disconnect all outputs in inputs nodes first -- eventhough we may
        //  end up adding them back in later
        //

        {
            HOP_PROF_DYN_NAME(
                std::string(std::to_string(m_inputs.size())
                            + std::string(" nodes - outputDisconnect"))
                    .c_str());

            for (size_t i = 0; i < m_inputs.size(); i++)
            {
                m_inputs[i]->outputDisconnect(this);
            }
        }

        {
            HOP_PROF("copying nodes in m_inputs)");
            m_inputs.resize(nodes.size());
            std::copy(nodes.begin(), nodes.end(), m_inputs.begin());
        }

        {
            HOP_PROF("addOutput loop");
            for (size_t i = 0; i < m_inputs.size(); i++)
            {
                if (!m_unconstrainedInputs && m_inputs[i]->group() != group())
                {
                    //
                    //  At this point we just issue a warning about not having
                    //  common groups. It might be better to throw in this
                    //  case.
                    //

                    cout << "WARNING: the input and output groups do not match:"
                         << endl;
                    cout << "         lhs node " << m_inputs[i]->name();
                    if (m_inputs[i]->group())
                        cout << " is member of "
                             << m_inputs[i]->group()->name();
                    else
                        cout << " is top level node";
                    cout << endl;
                    cout << "         rhs node " << name();
                    if (group())
                        cout << " is member of " << group()->name();
                    else
                        cout << " is top level node";
                    cout << endl;
                }

                //
                //  For N copies of an input there will be N copies at the
                //  output
                //

                m_inputs[i]->addOutput(this);
            }
        }

        // propagateInputChange();
        {
            HOP_PROF("m_inputsChangedSignal");
            m_inputsChangedSignal();
        }
        {
            HOP_PROF("graph()->inputsChanged");
            if (graph())
                graph()->inputsChanged(this);
        }
    }

    void IPNode::outputDisconnect(IPNode* node)
    {
        IPNodes::iterator i =
            std::find(m_outputs.begin(), m_outputs.end(), node);
        if (i != m_outputs.end())
            m_outputs.erase(i);
        m_outputsChangedSignal();
    }

    bool IPNode::isReady() const { return true; }

    void IPNode::addOutput(IPNode* node)
    {
        m_outputs.push_back(node);
        m_outputsChangedSignal();
    }

    IPNode::Context IPNode::contextForFrame(int frame)
    {
        return graph()->contextForFrame(frame);
    }

    IPImage* IPNode::evaluate(const Context& context)
    {
        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return IPImage::newNoImage(this, "No Input");
        IPImage* head = 0;

        try
        {
            if (nodes.size() == 1)
            {
                head = nodes.front()->evaluate(context);
            }
            else
            {
                head = new IPImage(this);

                for (size_t i = 0; i < nodes.size(); i++)
                {
                    IPImage* img = nodes[i]->evaluate(context);
                    head->appendChild(img);
                }

                head->recordResourceUsage();
            }
        }
        catch (exception& exc)
        {
            delete head;
            throw;
        }

        return head;
    }

    IPImageID* IPNode::evaluateIdentifier(const Context& context)
    {
        if (inputs().empty())
            return 0;

        if (inputs().size() == 1)
        {
            return inputs().front()->evaluateIdentifier(context);
        }
        else
        {
            IPImageID* idnode = new IPImageID;
            IPImageID* next = 0;

            for (size_t i = 0; i < inputs().size(); i++)
            {
                IPImageID* child = inputs()[i]->evaluateIdentifier(context);
                if (next)
                    next->next = child;
                next = child;
                if (!i)
                    idnode->children = child;
            }

            return idnode;
        }
    }

    IPNode::Matrix IPNode::localMatrix(const Context&) const
    {
        return Matrix();
    }

    // IPNode::Matrix
    // IPNode::localMatrixForInput(const Context&) const
    // {
    //     return Matrix();
    // }

    void IPNode::metaEvaluate(const Context& context, MetaEvalVisitor& visitor)
    {
        const IPNodes& nodes = inputs();

        visitor.enter(context, this);

        for (size_t i = 0; i < nodes.size(); i++)
        {
            IPNode* node = nodes[i];

            if (visitor.traverseChild(context, i, this, node))
            {
                node->metaEvaluate(context, visitor);
            }
        }

        visitor.leave(context, this);
    }

    void IPNode::visitRecursive(NodeVisitor& visitor)
    {
        const IPNodes& nodes = inputs();

        visitor.enter(this);

        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (visitor.traverseChild(i, this, nodes[i]))
                nodes[i]->visitRecursive(visitor);
        }

        visitor.leave(this);
    }

    void IPNode::testEvaluate(const Context& context,
                              TestEvaluationResult& result)
    {
        if (inputs().empty())
            return;

        for (size_t i = 0; i < inputs().size(); i++)
        {
            inputs()[i]->testEvaluate(context, result);
        }
    }

    IPNode::ImageRangeInfo IPNode::imageRangeInfo() const
    {
        if (m_inputs.size())
        {
            return m_inputs.front()->imageRangeInfo();
        }

        return ImageRangeInfo();
    }

    IPNode::ImageStructureInfo
    IPNode::imageStructureInfo(const Context& context) const
    {
        if (m_inputs.size())
        {
            return m_inputs.front()->imageStructureInfo(context);
        }

        return ImageStructureInfo();
    }

    void IPNode::mediaInfo(const Context& context, MediaInfoVector& infos) const
    {
        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->mediaInfo(context, infos);
        }
    }

    bool IPNode::isMediaActive() const
    {
        if (m_inputs.size())
        {
            return m_inputs.front()->isMediaActive();
        }

        return false;
    }

    void IPNode::setMediaActive(bool state)
    {
        if (!m_inputs.empty())
        {
            // If OCIO is enabled, then activating the media could potentially
            // pull the rug under our feet if the OCIO source-group-complete
            // handler were to replace the execution pipeline to introduce the
            // OCIO nodes. So we need to make a list before hand of the inputs
            // to be activated if multiple inputs were involved.
            if (m_inputs.size() == 1)
            {
                m_inputs.front()->setMediaActive(state);
            }
            else
            {
                IPNodes inputs = m_inputs;
                for (size_t i = 0; i < inputs.size(); i++)
                {
                    inputs[i]->setMediaActive(state);
                }
            }
        }
    }

    size_t IPNode::audioFillBuffer(const AudioContext& context)
    {
        //
        //  Default is just use the first input. (What else could you do
        //  for a default?)
        //

        if (m_inputs.size())
        {
            return m_inputs.front()->audioFillBuffer(context);
        }

        return 0;
    }

    void IPNode::propagateFlushToInputs(const FlushContext& context)
    {
        flushAllCaches(context);

        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->propagateFlushToInputs(context);
        }
    }

    void IPNode::propagateFlushToOutputs(const FlushContext& context)
    {
        flushAllCaches(context);

        for (size_t i = 0; i < m_outputs.size(); i++)
        {
            m_outputs[i]->propagateFlushToOutputs(context);
        }
    }

    void IPNode::propagateInputChange()
    {
        if (m_deleting)
            return;
        propagateInputChangeInternal();
    }

    void IPNode::propagateStateChange()
    {
        if (m_deleting)
            return;
        propagateStateChangeInternal();
    }

    void IPNode::propagateRangeChange(PropagateTarget target)
    {
        if (m_deleting)
            return;
        propagateRangeChangeInternal(target);

        if (target & GroupPropagateTarget)
        {
            if (group())
                group()->propagateRangeChange(target);
        }
    }

    void IPNode::propagateImageStructureChange(PropagateTarget target)
    {
        if (m_deleting)
            return;
        propagateImageStructureChangeInternal(target);

        if (target & GroupPropagateTarget)
        {
            if (group())
                group()->propagateImageStructureChange(target);
        }
    }

    void IPNode::propagateMediaChange(PropagateTarget target)
    {
        if (m_deleting)
            return;
        propagateMediaChangeInternal(target);

        if (target & GroupPropagateTarget)
        {
            if (group())
                group()->propagateMediaChange();
        }
    }

    void IPNode::propagateInputChangeInternal()
    {
        if (m_deleting)
            return;

        if (m_outputs.empty())
        {
            if (group())
                group()->propagateInputChangeInternal();
        }
        else
        {
            for (size_t i = 0; i < m_outputs.size(); i++)
            {
                m_outputs[i]->inputChanged(i);
                m_outputs[i]->propagateInputChangeInternal();
            }
        }
    }

    void IPNode::propagateStateChangeInternal()
    {
        if (m_deleting)
            return;

        m_graphStateChangedSignal();

        if (m_outputs.empty())
        {
            if (group())
                group()->propagateStateChangeInternal();
        }
        else
        {
            for (size_t i = 0; i < m_outputs.size(); i++)
            {
                m_outputs[i]->inputStateChanged(i);
                m_outputs[i]->propagateStateChangeInternal();
            }
        }
    }

    void IPNode::propagateRangeChangeInternal(PropagateTarget target)
    {
        if (m_deleting)
            return;

        m_rangeChangedSignal();

        if (m_outputs.empty())
        {
            //
            //  Root node notify the graph
            //
            if (target & GraphPropagateTarget)
            {
                if (!group() && graph())
                    graph()->rangeChanged(this);
            }
        }
        else
        {
            if (target & OutputPropagateTarget)
            {
                auto outputTarget = target;
                if ((target & GroupAndGraphInOutputPropagateTarget) == 0)
                    outputTarget = PropagateTarget(
                        target
                        & ~(GraphPropagateTarget | GroupPropagateTarget));

                int outputNum = 0;
                for (auto& currOutput : m_outputs)
                {
                    currOutput->inputRangeChanged(outputNum++, outputTarget);
                    currOutput->propagateRangeChangeInternal(outputTarget);
                }
            }
        }
    }

    void IPNode::propagateImageStructureChangeInternal(PropagateTarget target)
    {
        if (m_deleting)
            return;

        m_imageStructureChangedSignal();

        if (m_outputs.empty())
        {
            //
            //  Root node notify the graph
            //

            if (target & GraphPropagateTarget)
            {
                if (!group() && graph())
                {
                    graph()->imageStructureChanged(this);
                }
            }
        }
        else
        {
            if (target & OutputPropagateTarget)
            {
                auto outputTarget = target;
                if ((target & GroupAndGraphInOutputPropagateTarget) == 0)
                    outputTarget = PropagateTarget(
                        target
                        & ~(GraphPropagateTarget | GroupPropagateTarget));

                int outputNum = 0;
                for (auto& currOutput : m_outputs)
                {
                    currOutput->inputImageStructureChanged(outputNum++,
                                                           outputTarget);
                    currOutput->propagateImageStructureChangeInternal(
                        outputTarget);
                }
            }
        }
    }

    void IPNode::propagateMediaChangeInternal(PropagateTarget target)
    {
        if (m_deleting)
            return;

        m_mediaChangedSignal();

        if (m_outputs.empty())
        {
            //
            //  Root node notify the graph
            //
            if (target & GraphPropagateTarget)
            {
                if (!group() && graph())
                    graph()->mediaChanged(this);
            }
        }
        else
        {
            if (target & OutputPropagateTarget)
            {
                auto outputTarget = target;
                if ((target & GroupAndGraphInOutputPropagateTarget) == 0)
                    outputTarget = PropagateTarget(
                        target
                        & ~(GraphPropagateTarget | GroupPropagateTarget));
                int outputPin = 0;

                for (auto& currOutput : m_outputs)
                {
                    currOutput->inputMediaChanged(this, outputPin++,
                                                  outputTarget);
                    currOutput->propagateMediaChangeInternal(outputTarget);
                }
            }
        }
    }

    void IPNode::propagateAudioConfigToInputs(const AudioConfiguration& config)
    {
        audioConfigure(config);

        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->propagateAudioConfigToInputs(config);
        }
    }

    void IPNode::propagateGraphConfigToInputs(const GraphConfiguration& config)
    {
        graphConfigure(config);

        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->propagateGraphConfigToInputs(config);
        }
    }

    void IPNode::inputChanged(int) {}

    void IPNode::inputStateChanged(int) {}

    void IPNode::inputRangeChanged(int, PropagateTarget target) {}

    void IPNode::inputImageStructureChanged(int, PropagateTarget target) {}

    void IPNode::inputMediaChanged(IPNode* srcNode, int srcNodeInputNdx,
                                   PropagateTarget target)
    {
    }

    void IPNode::flushAllCaches(const FlushContext&) {}

    void IPNode::prepareForWrite() {}

    void IPNode::writeCompleted() {}

    void IPNode::readCompleted(const std::string&, unsigned int) {}

    void IPNode::audioConfigure(const AudioConfiguration&) {}

    void IPNode::graphConfigure(const GraphConfiguration&) {}

    void IPNode::newPropertyCreated(const Property* p)
    {
        m_newPropertySignal(p);
        m_graph->newPropertyCreated(p);
    }

    void IPNode::propertyChanged(const Property* p)
    {
        m_propertyChangedSignal(p);
        m_graph->propertyChanged(p);
    }

    void IPNode::propertyWillBeDeleted(const Property* p)
    {
        m_propertyWillBeDeletedSignal(p);
        m_graph->propertyWillBeDeleted(p);
    }

    void IPNode::propertyDeleted(const std::string& name)
    {
        m_propertyDeletedSignal(name);
        m_graph->propertyDeleted(name);
    }

    void IPNode::propertyWillChange(const Property* p)
    {
        m_propertyWillChangeSignal(p);
    }

    void IPNode::propertyWillInsert(const Property* p, size_t index,
                                    size_t size)
    {
        m_propertyWillInsertSignal(p, index, size);
        m_graph->propertyWillInsert(p, index, size);
    }

    void IPNode::propertyDidInsert(const Property* p, size_t index, size_t size)
    {
        m_propertyDidInsertSignal(p, index, size);
        m_graph->propertyDidInsert(p, index, size);
    }

    bool IPNode::testInputs(const IPNodes& nodes, ostringstream& msg) const
    {
        bool ok = true;
        const int maxNum = maximumNumberOfInputs();
        const int minNum = minimumNumberOfInputs();

        if (maxNum >= 0 && maxNum < nodes.size())
        {
            string un = uiName();
            string n = name();

            if (un == n)
                msg << n;
            else
                msg << un << " (" << n << ")";

            if (maxNum > 0)
            {
                msg << " accepts no more than " << maxNum << " input"
                    << (maxNum > 1 ? "s" : "") << endl;
            }
            else
            {
                msg << " cannot have inputs" << endl;
            }

            ok = false;
        }

        if (ok)
        {
            for (size_t i = 0; i < nodes.size(); i++)
            {
                if (!nodes[i]->testInputsInternal(this))
                {
                    string un = nodes[i]->uiName();
                    string n = nodes[i]->name();

                    if (un == n)
                        msg << n;
                    else
                        msg << un << " (" << n << ")";
                    msg << " would cause a cycle" << endl;

                    ok = false;
                }
            }
        }

        return ok;
    }

    bool IPNode::testInputsInternal(const IPNode* root) const
    {
        if (this == root)
            return false;

        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            if (!m_inputs[i]->testInputsInternal(root))
                return false;
        }

        return true;
    }

    IPNode::InputComparisonResult
    IPNode::compareToInputs(const IPNodes& nodes) const
    {
        if (m_inputs.size() <= nodes.size())
        {
            bool differ = false;

            for (size_t i = 0; i < m_inputs.size(); i++)
            {
                if (m_inputs[i] != nodes[i])
                {
                    differ = true;
                    break;
                }
            }

            if (!differ)
            {
                if (m_inputs.size() == nodes.size())
                    return IdenticalResult;
                else
                    return AppendedResult;
            }

            if (nodes.size() > m_inputs.size())
            {
                for (size_t i = 0; i < m_inputs.size(); i++)
                {
                    bool found = false;

                    for (size_t j = 0; j < nodes.size(); j++)
                    {
                        if (m_inputs[i] == nodes[j])
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                        return DiffersResult;
                }

                return AddedResult;
            }
        }

        if (m_inputs.size() == nodes.size())
        {
            IPNodes a = inputs();
            IPNodes b = nodes;

            sort(a.begin(), a.end());
            sort(b.begin(), b.end());

            for (size_t i = 0; i < a.size(); i++)
            {
                if (a[i] != b[i])
                    return DiffersResult;
            }

            return ReorderedResult;
        }

        return DiffersResult;
    }

    void IPNode::inputReordering(const IPNodes& newOrder,
                                 IndexArray& indices) const
    {
        size_t s = inputs().size();
        indices.resize(s);

        for (size_t i = 0; i < s; i++)
        {
            IPNode* input = inputs()[i];

            for (size_t j = 0; j < s; j++)
            {
                if (newOrder[j] == input)
                {
                    indices[i] = j;
                    break;
                }
            }
        }
    }

    void IPNode::reorder(const IndexArray& indices)
    {
        IPNodes newInputs(inputs().size());
        std::copy(inputs().begin(), inputs().end(), newInputs.begin());

        for (size_t i = 0; i < indices.size(); i++)
        {
            m_inputs[i] = newInputs[indices[i]];
        }

        m_inputsChangedSignal();
        if (graph())
            graph()->inputsChanged(this);
    }

    void IPNode::inputPartialReordering(const IPNodes& newOrder,
                                        IndexArray& removeIndices,
                                        IndexArray& reorderIndices) const
    {
        //
        //  NOTE: this function has to handle duplicate inputs. Hence the
        //  set of used indices.
        //

        size_t s = inputs().size();
        reorderIndices.resize(newOrder.size());
        set<int> used;

        fill(reorderIndices.begin(), reorderIndices.end(), -1);

        for (size_t i = 0; i < s; i++)
        {
            IPNode* input = inputs()[i];
            bool found = false;

            for (size_t j = 0; j < newOrder.size(); j++)
            {
                if (used.count(j) == 0 && newOrder[j] == input)
                {
                    reorderIndices[j] = i;
                    found = true;
                    used.insert(j);
                    break;
                }
            }

            if (!found)
                removeIndices.push_back(i);
        }
    }

    void IPNode::findInputs(const IndexArray& indices, IPNodes& nodes) const
    {
        nodes.clear();

        for (size_t i = 0; i < indices.size(); i++)
        {
            int index = indices[i];

            if (index >= 0)
            {
                nodes.push_back(inputs()[index]);
            }
        }
    }

    void IPNode::mapInputToEvalFrames(size_t inputIndex,
                                      const vector<int>& inframe,
                                      vector<int>& outframes) const
    {
        std::copy(inframe.begin(), inframe.end(), back_inserter(outframes));
    }

    //----------------------------------------------------------------------

    void IPNode::MetaEvalPath::enter(const Context& c, IPNode* n)
    {
        info.push_back(MetaEvalInfo(c.frame, n));
        if (n == leaf)
            found = true;
    }

    void IPNode::MetaEvalPath::leave(const Context& c, IPNode* n)
    {
        if (!found)
            info.pop_back();
    }

    bool IPNode::MetaEvalPath::traverseChild(const Context& c, size_t index,
                                             IPNode* parent, IPNode* child)
    {
        return child->isMetaSearchable() && !found;
    }

    void IPNode::MetaEvalClosestByTypeName::enter(const Context& c,
                                                  IPNode* node)
    {
        if (node->protocol() == typeName)
        {
            info.push_back(MetaEvalInfo(c.frame, node));
        }
    }

    bool IPNode::MetaEvalClosestByTypeName::traverseChild(const Context& c,
                                                          size_t index,
                                                          IPNode* parent,
                                                          IPNode* child)
    {
        return child->isMetaSearchable()
               && (info.empty() || info.back().node != parent);
    }

    void IPNode::MetaEvalFirstClosestByTypeName::enter(const Context& c,
                                                       IPNode* node)
    {
        if (node->protocol() == typeName)
        {
            info.push_back(MetaEvalInfo(c.frame, node));
        }
    }

    bool IPNode::MetaEvalFirstClosestByTypeName::traverseChild(const Context& c,
                                                               size_t index,
                                                               IPNode* parent,
                                                               IPNode* child)
    {
        return child->isMetaSearchable() && info.empty();
    }

    bool IPNode::PropertyAsFramesVisitor::traverseChild(size_t index,
                                                        IPNode* parent,
                                                        IPNode* child)
    {
        //
        //  NOTE: if the nodeMap already has a value for child then we've
        //  already visited it and its result is already cached.
        //

        return child->isMetaSearchable() && currentDepth <= depth
               && !nodeMap.count(child);
    }

    void IPNode::PropertyAsFramesVisitor::enter(IPNode* n)
    {
        if (const IntProperty* p = n->property<IntProperty>(propName))
        {
            nodeMap[n] = p->valueContainer();
            currentDepth++;
        }
    }

    void IPNode::PropertyAsFramesVisitor::leave(IPNode* n)
    {
        if (AdaptorIPNode* a = dynamic_cast<AdaptorIPNode*>(n))
        {
            if (nodeMap.count(a->groupInputNode()))
            {
                n->mapInputToEvalFrames(0, nodeMap[a->groupInputNode()],
                                        nodeMap[n]);
            }
        }
        else if (GroupIPNode* g = dynamic_cast<GroupIPNode*>(n))
        {
            if (nodeMap.count(g->rootNode()))
            {
                n->mapInputToEvalFrames(0, nodeMap[g->rootNode()], nodeMap[n]);
            }
        }
        else
        {
            const IPNode::IPNodes& children = n->inputs();

            for (size_t i = 0; i < children.size(); i++)
            {
                IPNode* child = children[i];

                if (nodeMap.count(child))
                {
                    n->mapInputToEvalFrames(i, nodeMap[child], nodeMap[n]);
                }
            }
        }

        if (const IntProperty* p = n->property<IntProperty>(propName))
        {
            currentDepth--;
        }
    }

    bool IPNode::ClosestByTypeNameVisitor::traverseChild(size_t index,
                                                         IPNode* parent,
                                                         IPNode* child)
    {
        return child->isMetaSearchable() && currentDepth <= maxDepth;
    }

    void IPNode::ClosestByTypeNameVisitor::enter(IPNode* n)
    {
        if (n->protocol() == name)
        {
            nodes.insert(n);
            currentDepth++;
        }
    }

    void IPNode::ClosestByTypeNameVisitor::leave(IPNode* n)
    {
        if (n->protocol() == name)
        {
            currentDepth--;
        }
    }

    void IPNode::willDelete()
    {
        m_deleting = true;
        m_willDeleteSignal();
    }

    void IPNode::collectInputs(IPNodes& nodes)
    {
        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            m_inputs[i]->collectInputs(nodes);
        }

        nodes.push_back(this);
    }

    IPNode* IPNode::inputs1() const
    {
        if (inputs().empty())
            return 0;
        else
            return inputs()[0];
    }

    IPNode* IPNode::inputs2() const
    {
        if (inputs().size() < 2)
            return 0;
        else
            return inputs()[1];
    }

    IPNode* IPNode::inputs3() const
    {
        if (inputs().size() < 3)
            return 0;
        else
            return inputs()[2];
    }

    IPNode* IPNode::inputs4() const
    {
        if (inputs().size() < 4)
            return 0;
        else
            return inputs()[3];
    }

    void IPNode::setInputs1(IPNode* node0)
    {
        IPNodes inputs(1);
        inputs[0] = node0;
        setInputs(inputs);
    }

    void IPNode::setInputs2(IPNode* node0, IPNode* node1)
    {
        IPNodes inputs(2);
        inputs[0] = node0;
        inputs[1] = node1;
        setInputs(inputs);
    }

    void IPNode::setInputs3(IPNode* node0, IPNode* node1, IPNode* node2)
    {
        IPNodes inputs(2);
        inputs[0] = node0;
        inputs[1] = node1;
        inputs[2] = node2;
        setInputs(inputs);
    }

    void IPNode::setInputs4(IPNode* node0, IPNode* node1, IPNode* node2,
                            IPNode* node3)
    {
        IPNodes inputs(2);
        inputs[0] = node0;
        inputs[1] = node1;
        inputs[2] = node2;
        inputs[3] = node3;
        setInputs(inputs);
    }

    IPImage::ResourceUsage IPNode::accumulate(const IPImageVector& images)
    {
        IPImage::ResourceUsage usage;

        for (size_t i = 0; i < images.size(); i++)
        {
            usage.accumulate(images[i]->resourceUsage);
        }

        return usage;
    }

    IPImage::ResourceUsage IPNode::filterAccumulate(const IPImageVector& images)
    {
        IPImage::ResourceUsage usage;

        for (size_t i = 0; i < images.size(); i++)
        {
            usage.filterAccumulate(images[i]->resourceUsage);
        }

        return usage;
    }

    namespace
    {

        typedef size_t (*FieldFunc)(const IPImage::ResourceUsage&);

        size_t returnBuffersFunc(const IPImage::ResourceUsage& u)
        {
            return u.buffers;
        }

        size_t returnCoordsFunc(const IPImage::ResourceUsage& u)
        {
            return u.coords;
        }

        size_t returnFetchesFunc(const IPImage::ResourceUsage& u)
        {
            return u.fetches;
        }

        struct CompareCount
        {
            CompareCount(FieldFunc F)
                : field(F)
            {
            }

            FieldFunc field;

            bool operator()(const IPImage* a, const IPImage* b)
            {
                return field(a->resourceUsage) >= field(b->resourceUsage);
            }
        };

        bool assignByResourceCount(const IPNode* node, FieldFunc field,
                                   const IPImage::ResourceUsage& usage,
                                   IPImageVector& images, IPImageSet& modified,
                                   size_t maxValue)
        {
            bool modifiedSomething = false;
            size_t ncurrent = field(usage);
            if (ncurrent <= maxValue)
                return false;

            IPImageVector sortedImages(images);

            sort(sortedImages.begin(), sortedImages.end(), CompareCount(field));

            for (size_t i = 0; i < sortedImages.size() && ncurrent > maxValue;
                 i++)
            {
                IPImage* img = sortedImages[i];

                size_t icount = field(img->resourceUsage);

                if (icount > 1)
                {
                    if (img->mergeExpr)
                    {
                        ncurrent -= field(img->resourceUsage) - 1;
                        img->destination = IPImage::IntermediateBuffer;
                        img->resourceUsage.set(1, 1, 1);
                        modified.insert(img);
                        modifiedSomething = true;
                    }
                    else if (img->shaderExpr)
                    {
                        ncurrent -= field(img->resourceUsage) - 1;
                        img->destination = IPImage::IntermediateBuffer;
                        img->resourceUsage.set(1, 1, 1);
                        modifiedSomething = true;
                    }
                }
            }

            return modifiedSomething;
        }

    } // namespace

    // we need to insert an intermediate IPImage at the top of the IPTree
    // if there is any IPImage with paint commands that doesn't have an
    // intermediate IPImage has an ancestor. this is because if there is no
    // intermediate, everyone will render to opengl default framebuffer. This
    // makes it not possible for paint to use the content of the existing render
    // as a texture (as an erase stroke might). an alternative to this is to
    // copy the default framebuffer content to a framebuffer object in
    // renderPaint, but that would be more expensive.
    bool needToInsertIntermediateRecursive(const IPImage* root)
    {
        if (!root->commands.empty())
            return true;

        if (root->destination != IPImage::IntermediateBuffer && root->children)
        {
            bool needToInsert =
                needToInsertIntermediateRecursive(root->children);
            if (needToInsert)
                return true;
        }

        if (root->next)
            return needToInsertIntermediateRecursive(root->next);

        return false;
    }

    IPImage*
    IPNode::insertIntermediateRendersForPaint(IPImage* root,
                                              const Context& context) const
    {
        if (root->destination == IPImage::IntermediateBuffer)
        {
            return root;
        }

        bool needToInsert = true;
        if (root->children)
            needToInsert = needToInsertIntermediateRecursive(root->children);
        if (!needToInsert)
            return root;

        IPImage* nroot =
            new IPImage(this, IPImage::BlendRenderType, context.viewWidth,
                        context.viewHeight, 1.0, IPImage::IntermediateBuffer,
                        IPImage::FloatDataType);

        root->fitToAspect(nroot->displayAspect());
        nroot->appendChild(root);
        nroot->shaderExpr = Shader::newSourceRGBA(nroot);
        nroot->resourceUsage =
            nroot->shaderExpr->computeResourceUsageRecursive();
        return nroot;
    }

    void IPNode::balanceResourceUsage(UsageFunction accumFunc,
                                      IPImageVector& images,
                                      IPImageSet& modified, size_t maxBuffers,
                                      size_t maxCoords, size_t maxFetches,
                                      size_t incomingSamplers) const
    {
        IPImage::ResourceUsage usage = accumFunc(images);

        //
        //  First check limit of texture buffers (images)
        //

        if (assignByResourceCount(this, returnBuffersFunc, usage, images,
                                  modified, maxBuffers + incomingSamplers))
        {
            accumFunc(images);
        }

        //
        //  Next limit of coordinates
        //

        if (assignByResourceCount(this, returnCoordsFunc, usage, images,
                                  modified, maxCoords))
        {
            accumFunc(images);
        }

        //
        //  The number of fetches (this one is subjective and may need to be
        //  dynamically determined depending on the hardware).
        //

        assignByResourceCount(this, returnFetchesFunc, usage, images, modified,
                              maxFetches);
    }

    bool IPNode::willConvertToIntermediate(IPImage* img)
    {
        return (img->renderType == IPImage::BlendRenderType && !img->shaderExpr
                && !img->mergeExpr && img->children && !img->noIntermediate);
    }

    bool IPNode::convertBlendRenderTypeToIntermediate(IPImage* img)
    {
        if (willConvertToIntermediate(img))
        {
            img->destination = IPImage::IntermediateBuffer;
            img->shaderExpr = Shader::newSourceRGBA(img);
            img->resourceUsage =
                img->shaderExpr->computeResourceUsageRecursive();

            return true;
        }

        return false;
    }

    bool
    IPNode::convertBlendRenderTypeToIntermediate(const IPImageVector& images,
                                                 IPImageSet& modifiedImages)
    {
        for (size_t i = 0; i < images.size(); i++)
        {
            if (convertBlendRenderTypeToIntermediate(images[i]))
            {
                modifiedImages.insert(images[i]);
            }
        }

        return !modifiedImages.empty();
    }

    void
    IPNode::assembleMergeExpressions(IPImage* root, IPImageVector& inImages,
                                     const IPImageSet& modifiedImages,
                                     bool isFilter,
                                     Shader::ExpressionVector& inExpressions)
    {
        for (size_t i = 0; i < inImages.size(); i++)
        {
            //
            //  if the current image has paint commands, generate a new
            //  IPImage whose child will be image.
            //
            //  -or-
            //
            //  If the image has a data window we need to do the same
            //  thing.
            //

            if (!inImages[i]->commands.empty()
                || (inImages[i]->fb && inImages[i]->fb->needsUncrop()))
            {
                IPImage* newImage = new IPImage(
                    this, IPImage::BlendRenderType, inImages[i]->width,
                    inImages[i]->height, 1.0, IPImage::IntermediateBuffer,
                    IPImage::FloatDataType);
                newImage->children = inImages[i];
                newImage->shaderExpr = Shader::newSourceRGBA(newImage);
                newImage->resourceUsage =
                    newImage->shaderExpr->computeResourceUsageRecursive();
                inImages[i] = newImage;
            }

            IPImage* image = inImages[i];

            if (modifiedImages.count(image) > 0)
            {
                //
                //  Don't touch images that were modified by
                //  balanceResourceUsage() etc. These were changed to
                //  prevent resource usage limits being broken.
                //

                inExpressions.push_back(image->shaderExpr);
                image->shaderExpr = 0;
            }
            else if (image->mergeExpr)
            {
                if (isFilter
                    && Shader::sourceFunctionCount(image->mergeExpr, 2) == 1
                    && Shader::filterFunctionCount(image->mergeExpr, 1) == 0)
                {
                    inExpressions.push_back(replaceSourceWithExpression(
                        image->shaderExpr, image->mergeExpr));
                    image->mergeExpr = 0;
                    image->shaderExpr = 0;
                    image->destination = IPImage::CurrentFrameBuffer;
                }
                else
                {
                    inExpressions.push_back(image->shaderExpr);
                    image->destination = IPImage::IntermediateBuffer;
                    image->shaderExpr = 0;
                    image->blendMode = IPImage::Replace;
                }
            }
            else if (image->destination == IPImage::IntermediateBuffer
                     || image->destination == IPImage::DataBuffer)
            {
                inExpressions.push_back(image->shaderExpr);
                image->shaderExpr = 0;
            }
            else
            {
                //
                //  Is this case correct?
                //

                inExpressions.push_back(image->shaderExpr);
                image->shaderExpr = 0;
                image->destination = IPImage::CurrentFrameBuffer;
            }
        }
    }

    void IPNode::filterLimits(const IPImageVector& inImages,
                              IPImageSet& modifiedImages)
    {
        //
        //  This prevents filters of filters in cases where the performance
        //  would degrade.
        //
    }

    void IPNode::collectMemberNodes(IPNodeSet& nodeSet, size_t depth)
    {
        if (depth != 0)
            nodeSet.insert(this);
    }

    //
    // Returns the unqualified form a layer or channel property name.
    //  e.g. "right|diffuse|R" returns "R".
    //
    std::string IPNode::getUnqualifiedName(const std::string& name)
    {
        size_t pos = name.rfind("|");
        if (pos != string::npos)
        {
            string result = name.substr(pos + 1);
            return result;
        }
        else
        {
            return name;
        }
    }

    std::ostream& operator<<(std::ostream& o, const IPNode::ImageComponent& c)
    {
        switch (c.type)
        {
        default:
        case IPNode::NoComponent:
            o << "None:";
            break;
        case IPNode::ViewComponent:
            o << "View:";
            break;
        case IPNode::LayerComponent:
            o << "Layer:";
            break;
        case IPNode::ChannelComponent:
            o << "Channel:";
            break;
        }
        for (size_t i = 0; i < c.name.size(); ++i)
        {
            o << c.name[i];
            if (i < c.name.size() - 1)
                o << ",";
        }

        return o;
    }

    void IPNode::isolate()
    {
        //
        //  Get the node's inputs and store those
        //

        if (!component("__graph"))
        {
            StringProperty* sp =
                createProperty<StringProperty>("__graph.inputs");
            sp->resize(m_inputs.size());
            for (size_t i = 0; i < m_inputs.size(); i++)
                (*sp)[i] = m_inputs[i]->name();

            //
            //  Same with the outputs
            //

            sp = createProperty<StringProperty>("__graph.outputs");
            IntProperty* ip =
                createProperty<IntProperty>("__graph.outputIndex");
            sp->resize(m_outputs.size());
            ip->resize(m_outputs.size());

            for (size_t i = 0; i < m_outputs.size(); i++)
            {
                (*sp)[i] = m_outputs[i]->name();
                (*ip)[i] = m_outputs[i]->indexOfChild(this);
            }
        }

        //
        //  Remove outputs and inputs
        //

        disconnectInputs();
        disconnectOutputs();

        //
        //  NOTE: after this call graph() will still have the IPGraph that
        //  used to own this node but the IPGraph will no longer know
        //  about this node
        //

        IPGraph* g = graph();
        g->removeNode(this);
        g->addIsolatedNode(this);
    }

    void IPNode::restore()
    {
        Component* comp = component("__graph");
        StringProperty* ins = comp->property<StringProperty>("inputs");
        StringProperty* outs = comp->property<StringProperty>("outputs");
        IntProperty* outIndex = comp->property<IntProperty>("outputIndex");

        graph()->addNode(this);
        graph()->removeIsolatedNode(this);

        //
        //  Restore the node's inputs. The order is important
        //

        IPNode::IPNodes newInputs;

        for (size_t i = 0; i < ins->size(); i++)
        {
            string name = (*ins)[i];

            if (IPNode* n = graph()->findNodePossiblyIsolated(name))
            {
                newInputs.push_back(n);
            }
            else
            {
                cout << "ERROR: " << __FUNCTION__ << " can't find input node \""
                     << name << "\"" << endl;
            }
        }

        setInputs(newInputs);

        //
        //  Connect the outputs of this node. Note that we need to restore
        //  the exact order of their inputs.
        //

        for (size_t i = 0; i < outs->size(); i++)
        {
            string name = (*outs)[i];

            if (IPNode* outNode = graph()->findNodePossiblyIsolated(name))
            {
                size_t index = (*outIndex)[i];

                //
                //  Just try and be sane here.
                //

                if (index > outNode->inputs().size())
                {
                    outNode->appendInput(this);
                }
                else
                {
                    outNode->insertInput(this, index);
                }
            }
            else
            {
                cout << "ERROR: " << __FUNCTION__
                     << " can't find output node \"" << name << "\"" << endl;
            }
        }

        // remove(comp);
        // delete comp;
    }

    bool IPNode::isIsolated() const
    {
        return graph() && graph()->m_isolatedNodes.count(name()) > 0;
    }

    void IPNode::setGraph(IPGraph* graph) { m_graph = graph; }

    void IPNode::undoDeref()
    {
        assert(m_undoRefCount > 0);
        m_undoRefCount--;
        if (m_undoRefCount == 0)
            delete this;
    }

    int IPNode::mapToInputIndex(IPNode* srcNode, int srcOutIndex) const
    {
        auto inputNdx = 0;
        for (auto curInput : inputs())
        {
            if (curInput == srcNode)
            {
                return inputNdx;
            }

            inputNdx++;
        }
        return -1;
    }

} // namespace IPCore
