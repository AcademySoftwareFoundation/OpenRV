//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__GroupIPNode__h__
#define __IPCore__GroupIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{
    class AdaptorIPNode;

    /// GroupIPNode manages a sub-graph

    ///
    /// A group node lives in the evaluation path on the graph and
    /// contains a sub-graph which is evaluated when the group node is
    /// evaluated. The sub-graph is visible to the IPGraph but does not
    /// appear connected to the graph outside of the GroupIPNode
    ///
    /// Some of the flow control is implemented in AdaptorIPNode which
    /// will call back into GroupIPNode to continue propgatation to inputs
    /// or outputs. The AdaptorIPNode should be created by the GroupIPNode
    /// for its sub-graph leaves and roots.
    ///

    class GroupIPNode : public IPNode
    {
    public:
        //
        //  NOTE: this class will delete its "root" node so don't do that
        //  in your derived class.
        //

        GroupIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group = 0);

        virtual ~GroupIPNode();

        //
        //  GroupIPNode API
        //

        void add(IPNode*);
        void remove(IPNode*);

        const IPNodeSet& members() const { return m_members; }

        virtual void copyNode(const IPNode*);

        IPNode* memberByType(const std::string& typeName);

        template <typename T>
        T* memberByTypeNameOfType(const std::string& typeName)
        {
            return dynamic_cast<T*>(memberByType(typeName));
        }

        void internalOutputNodesFor(IPNode*, IPNodes&) const;

        IPNode* rootNode() const { return m_root; }

        //
        //  If this group has additional outputs (like UI textures, etc)
        //  than they should have been indicated with addAuxillaryOutput()
        //
        //  NOTE: group members that are also groups with aux outputs
        //  should have their aux outputs added using addAuxillaryOutput()
        //  as well.
        //

        const IPNodes& auxillaryOutputNodes() const { return m_auxOutputs; }

        //
        //  Use this in order to make consistant internal node names
        //

        std::string internalNodeNameForInput(IPNode* input,
                                             const std::string& smallIdString);

        std::string internalNodeName(const std::string& idstring);

        //
        //  Remove IDs from cache that contain groupNode name as substr.
        //
        void flushIDsOfGroup();

        //
        //  This function is critical to hooking up the group node and
        //  each sub-class will require a special implementation. The
        //  easiest way to do this is to implement newSubGraphForInput()
        //  and possibly modifySubGraphForInput() and call
        //  setInputsWithReordering().
        //

        virtual void setInputs(const IPNodes&);

        //
        //  Reimplement if you call setInputsWithReordering(). Defaults to
        //  returning the input node from newInputs. The subgraph should
        //  be build for index of newInputs. The entire newInputs vector
        //  is passed in because you may need to refer to properties of
        //  the whole set (like union of input ranges, etc).
        //

        virtual IPNode* newSubGraphForInput(size_t index,
                                            const IPNodes& newInputs);

        //
        //  Similar to above, but in the case where an existing input is
        //  being reordered instead of created from scratch. Implementing
        //  this will give the subclass the opportunity to modify the
        //  existing passed-in subgraph or create a new one from
        //  scratch. If a new one is created the passed in subgraph should
        //  be deleted by the implementor.
        //

        virtual IPNode* modifySubGraphForInput(size_t index,
                                               const IPNodes& newInputs,
                                               IPNode* subgraph);

        //
        //  Can be called by setInputs() to make life easier (where
        //  possible). This function assumes your group as a single fan-in
        //  node which inputs that are sub-graphs each of which (probably)
        //  has an AdaptorIPNode as the leaf. The fanInNode inputs are
        //  moved around if possible so as to preserve the sub-graph
        //  state. newSubGraphForInput() is called to create new inputs
        //  for fanInNode from new inputs. Old unused inputs of the
        //  fanInNode are deleted.
        //  This operation is optimized for one input onlye if the
        //  inputIndexOnly is greater than zero.

        virtual void setInputsWithReordering(const IPNodes&, IPNode* fanInNode,
                                             int inputIndexOnly = -1);

        //
        //  The rest of the IPNode API with special implementations that
        //  pass control to the sub-graph
        //

        virtual ImageRangeInfo imageRangeInfo() const;
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void mediaInfo(const Context&, MediaInfoVector&) const;
        virtual bool isMediaActive() const;
        virtual void setMediaActive(bool state);
        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void visitRecursive(NodeVisitor&);

        virtual void testEvaluate(const Context&, TestEvaluationResult&);
        virtual void propagateFlushToInputs(const FlushContext&);
        virtual size_t audioFillBuffer(const AudioContext&);
        virtual void propagateAudioConfigToInputs(const AudioConfiguration&);
        virtual void propagateGraphConfigToInputs(const GraphConfiguration&);
        virtual void inputChanged(int index);
        virtual void inputRangeChanged(int index, PropagateTarget target);
        virtual void inputImageStructureChanged(int index,
                                                PropagateTarget target);
        virtual void collectMemberNodes(IPNodeSet&, size_t depth = 0);

        virtual void collectMemberNodesByTypeName(const std::string& typeName,
                                                  IPNodeSet&, size_t depth = 0);

        //
        //  NOTE: readCompleted() will be called during profile reading
        //  just as it is when a session file is read. That means any
        //  sub-graph reconfiguration should happen just like it does with
        //  session file reading.
        //

        virtual void readCompleted(const std::string&, unsigned int);

        static void swapNodes(IPNode* current, IPNode* replacement);

        //
        //  The group will recursively all these on its members
        //

        virtual void isolate();
        virtual void restore();

    protected:
        void setRoot(IPNode* n) { m_root = n; }

        void addAuxillaryOutput(IPNode*);

        IPNode* newMemberNode(const std::string& typeName,
                              const std::string& nameStem);

        IPNode* newMemberNodeForInput(const std::string& typeName,
                                      IPNode* input,
                                      const std::string& smallName);

        template <class T>
        T* newMemberNodeOfType(const std::string& typeName,
                               const std::string& nameStem)
        {
            return dynamic_cast<T*>(newMemberNode(typeName, nameStem));
        }

        template <class T>
        T* newMemberNodeOfTypeForInput(const std::string& typeName,
                                       IPNode* input,
                                       const std::string& smallIdString)
        {
            return dynamic_cast<T*>(
                newMemberNodeForInput(typeName, input, smallIdString));
        }

        AdaptorIPNode* newAdaptorForInput(IPNode* input);
        static void copyInputs(IPNode*, const IPNode*);

    protected:
        //
        //  Sub-classes should make sure this is set correctly
        //

        IPNode* m_root;
        IPNodeSet m_members;
        IPNodes m_auxOutputs;
    };

} // namespace IPCore

#endif // __IPCore__GroupIPNode__h__
