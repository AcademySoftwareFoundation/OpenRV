//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/SetInputs.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        SetInputsInfo::SetInputsInfo(const std::string& name,
                                     TwkApp::CommandInfo::UndoType type)
            : CommandInfo(name, type)
        {
        }

        SetInputsInfo::~SetInputsInfo() {}

        TwkApp::Command* SetInputsInfo::newCommand() const
        {
            return new SetInputs(this);
        }

        //----------------------------------------------------------------------

        SetInputs::SetInputs(const SetInputsInfo* info)
            : Command(info)
            , m_graph(0)
        {
        }

        SetInputs::~SetInputs() {}

        void SetInputs::setArgs(IPGraph* graph, const string& name,
                                const StringVector& inputs)
        {
            m_graph = graph;
            m_name = name;

            if (IPNode* node = graph->findNode(name))
            {
                m_inputs = inputs;
            }
            else
            {
                TWK_THROW_EXC_STREAM("SetInputs: unknown node " << name);
            }
        }

        void SetInputs::doit()
        {
            if (IPNode* node = m_graph->findNode(m_name))
            {
                IPNode::IPNodes newInputs(m_inputs.size());
                IPNode::IPNodes oldInputs = node->inputs();

                for (size_t i = 0; i < m_inputs.size(); i++)
                {
                    newInputs[i] = m_graph->findNode(m_inputs[i]);
                }

                node->setInputs(newInputs);

                m_inputs.resize(oldInputs.size());

                for (size_t i = 0; i < oldInputs.size(); i++)
                {
                    m_inputs[i] = oldInputs[i]->name();
                }
            }
        }

        void SetInputs::undo() { doit(); }

    } // namespace Commands
} // namespace IPCore
