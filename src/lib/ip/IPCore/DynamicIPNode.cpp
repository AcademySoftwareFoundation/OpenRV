//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/DynamicIPNode.h>
#include <IPCore/IPProperty.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/NodeManager.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;
    using namespace boost;
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    DynamicIPNode::DynamicIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* g,
                                 GroupIPNode* group)
        : GroupIPNode(name, def, g, group)
        , m_glslValid(0)
        , m_glslCurrent(0)
        , m_definition(0)
        , m_internalNode(0)
        , m_internalParameters(0)
    {
        PropertyInfo* noSave = new PropertyInfo(PropertyInfo::NotPersistent);
        PropertyInfo* outputOnly = new PropertyInfo(PropertyInfo::OutputOnly);
        PropertyInfo* programFlush = new PropertyInfo(
            PropertyInfo::RequiresProgramFlush | PropertyInfo::Persistent);

        m_parameters = createComponent("parameters");
        m_glslCurrent =
            declareProperty<StringProperty>("function.glsl", "", programFlush);
        m_glslValid =
            declareProperty<StringProperty>("function.glsl_valid", "");

        m_glslCurrent->front() =
            "//\n// Default Definition\n//\n\nvec4 main(const in inputImage "
            "in0)\n{\n    return in0() + vec4(1,.5,0,1);\n}\n";
        declareProperty<IntProperty>("function.fetches", 1);
        declareProperty<IntProperty>("render.intermediate", 0);
        declareProperty<StringProperty>("node.evaluationType", "color");
        declareProperty<StringProperty>("function.name", "main", noSave);
        declareProperty<StringProperty>("node.defaultName", "dynamicInternal",
                                        noSave);
        declareProperty<StringProperty>("node.origin", "dynamic", noSave);
        declareProperty<StringProperty>("node.export", "");
        declareProperty<StringProperty>("node.author", "");
        declareProperty<StringProperty>("node.company", "");
        declareProperty<StringProperty>("node.comment", "");
        declareProperty<StringProperty>("node.date", "");
        declareProperty<IntProperty>("node.userVisible", 1);
        m_definitionName = declareProperty<StringProperty>(
            "node.name", "AGoodNameForANodeType", noSave);

        declareProperty<StringProperty>("documentation.html",
                                        "Its a very nice node");
        declareProperty<StringProperty>("documentation.summary",
                                        "This node does something interesting");

        m_outputValid =
            declareProperty<IntProperty>("output.valid", 1, outputOnly);
        m_outputError =
            declareProperty<IntProperty>("output.error", 0, outputOnly);
        m_outputMessage =
            declareProperty<StringProperty>("output.message", "", outputOnly);
        m_outputMain = declareProperty<StringProperty>("output.functionName",
                                                       "", outputOnly);
        m_outputCallMain = declareProperty<StringProperty>(
            "output.functionCallName", "", outputOnly);
    }

    DynamicIPNode::~DynamicIPNode()
    {
        //
        //  Delete the internal node (the root node) and clean that up so we
        //  can delete the unmanaged node definition. Otherwise we have to
        //  assume that the deleted node definition is not used during deletion
        //  of m_internalNode (the root)
        //

        delete m_internalNode;
        m_internalNode = 0;
        setRoot(0);
        delete m_definition;
    }

    void DynamicIPNode::setInputs(const IPNodes& inputs)
    {
        GroupIPNode::setInputs(inputs);
        rebuild();
    }

    string DynamicIPNode::dynamicName() const
    {
        ostringstream str;
        str << this;
        return str.str();
    }

    void DynamicIPNode::publish(const string& filename)
    {
        NodeManager::NodeDefinitionVector defs(1);
        defs[0] = m_definition;
        setStringValue("node.export", filename);

        static const char* copyProps[] = {"node.name",
                                          "node.export",
                                          "node.author",
                                          "node.company",
                                          "node.comment",
                                          "documentation.html",
                                          "documentation.summary",
                                          0};

        for (const char** c = copyProps; *c; c++)
        {
            m_definition->setProperty<StringProperty>(*c, stringValue(*c));
        }

        m_definition->setName(stringValue("node.name"));

        ostringstream datestr;
        ptime now = second_clock::local_time();
        datestr << now;

        m_definition->declareProperty<StringProperty>("node.date",
                                                      datestr.str(), 0, true);
        m_definition->removeProperty<StringProperty>("function.glsl_valid");

        vector<string> removeComps(4);
        removeComps[0] = "session";
        removeComps[1] = "sm_state";
        removeComps[2] = "ui";
        removeComps[3] = "parameters";

        for (size_t i = 0; i < removeComps.size(); i++)
        {
            if (Component* c = m_definition->component(removeComps[i]))
            {
                m_definition->remove(c);
                delete c;
            }
        }

        m_definition->add(m_parameters->copy());

        graph()->nodeManager()->writeDefinitions(filename, defs, true);
    }

    void DynamicIPNode::setOutput(bool valid, bool error, const string& str)
    {
        setProperty<IntProperty>("output.valid", valid ? 1 : 0);
        setProperty<IntProperty>("output.error", error ? 1 : 0);
        setProperty<StringProperty>("output.message", str);
    }

    void DynamicIPNode::setOutputNames(const string& name,
                                       const string& callName)
    {
        setProperty<StringProperty>("output.functionName", name);
        setProperty<StringProperty>("output.functionCallName", callName);
    }

    void DynamicIPNode::rebuild()
    {
        setOutput(true, false, "");
        NodeDefinition* def = 0;

        try
        {
            //
            //  Try to create a definition based on this nodes properties
            //

            def = new NodeDefinition(this);
            def->reify();
        }
        catch (std::exception& exc)
        {
            setOutput(false, true, exc.what());
            return;
        }

        //
        //  Validate GLSL
        //

        const Shader::Function* F = def->function();

        if (!F)
        {
            string message =
                def->stringValue("error.message", "unable to build definition");
            setOutput(false, true, message);
            delete def;
            return;
        }

        IPGraph::NodeValidation validation(this);
        Shader::Function::ValidationResult result = F->validate();
        setOutput(result.first, result.second != "", result.second);
        setOutputNames(F->name(), F->callName());

        if (!result.first)
        {
            delete def;
            return;
        }

        //
        //  At this point assume its valid. In the case where we couldn't
        //  validate it directly just assume its ok.
        //

        delete m_definition;
        m_definition = def;
        m_glslValid->copy(m_glslCurrent);

        string evalType = m_definition->stringValue("node.evaluationType");
        setStringValue("node.evaluationType", evalType, true);

        //
        //  Attempt to create the internal node
        //

        try
        {
            m_internalParameters = 0;
            delete m_internalNode; // deletes the adaptors too.
            m_internalNode = def->newNode("dynamicInternal", graph(), this);
            m_internalNode->setWritable(false);
            m_internalParameters = m_internalNode->component("parameters");
        }
        catch (std::exception& exc)
        {
            setOutput(false, true, exc.what());
            return;
        }

        //
        //  create/find parameters
        //

        // const Shader::SymbolVector& params = F->parameters();
        set<Property*> usedProps;

        const Component* paramComp = m_internalNode->component("parameters");
        const Component::Container& pprops = paramComp->properties();

        for (size_t i = 0; i < pprops.size(); i++)
        {
            Property* iprop = pprops[i];
            Property* prop = m_parameters->find(iprop->name());
            Property* newProp = 0;

            if (!prop || !iprop->structureCompare(prop))
            {
                newProp = iprop->copy();
                if (prop)
                    m_parameters->remove(prop);
                m_parameters->add(newProp);
                prop = newProp;
            }

            if (prop)
            {
                prop->resize(1);
                usedProps.insert(prop);
            }
        }

        //
        //  Clean up unused parameters
        //

        const Component::Container& props = m_parameters->properties();
        vector<Property*> unusedProps;

        for (size_t i = 0; i < props.size(); i++)
        {
            if (usedProps.count(props[i]) == 0)
                unusedProps.push_back(props[i]);
        }

        for (size_t i = 0; i < unusedProps.size(); i++)
        {
            m_parameters->remove(unusedProps[i]);
        }

        //
        //  Hook up the instance to this nodes inputs.
        //

        const IPNodes& ins = inputs();
        size_t numFunctionParams = F->imageParameters().size();

        IPNodes internalInputs;

        for (size_t i = 0; i < ins.size() && i < numFunctionParams; i++)
        {
            AdaptorIPNode* anode = newAdaptorForInput(ins[i]);
            internalInputs.push_back(anode);
        }

        m_internalNode->setInputs(internalInputs);
        setRoot(m_internalNode);

        if (evalType == "transition")
        {
            ostringstream str;

            if (numFunctionParams != 2)
            {
                str << "WARNING: transition requires 2 inputs, but "
                    << numFunctionParams << " inputImage parameter"
                    << (numFunctionParams > 1 ? "s were " : " was ") << "found"
                    << endl;
            }

            bool startFrameFound = false;
            bool numFramesFound = false;
            bool frameFound = false;

            for (size_t i = 0; i < F->parameters().size(); i++)
            {
                string name = F->parameters()[i]->name();
                Shader::Symbol::Type type = F->parameters()[i]->type();
                if (name == "startFrame" && type == Shader::Symbol::FloatType)
                    startFrameFound = true;
                else if (name == "numFrames"
                         && type == Shader::Symbol::FloatType)
                    numFramesFound = true;
                else if (name == "frame" && type == Shader::Symbol::FloatType)
                    frameFound = true;
            }

            if (!startFrameFound)
            {
                str << "WARNING: transition requires float startFrame parameter"
                    << endl;
            }

            if (!numFramesFound)
            {
                str << "WARNING: transition requires float numFrames parameter"
                    << endl;
            }

            if (!frameFound)
            {
                str << "WARNING: transition requires float frame parameter"
                    << endl;
            }

            setOutput(true, true, str.str());
        }
        else if (ins.size() != numFunctionParams && evalType != "combine")
        {
            ostringstream str;
            str << "WARNING: too "
                << (ins.size() < numFunctionParams ? "few" : "many")
                << " inputs: found " << F->imageParameters().size()
                << " inputImage parameter"
                << (F->imageParameters().size() > 1 ? "s" : "")
                << " but node has " << inputs().size() << " input"
                << (inputs().size() > 1 ? "s" : "");
            setOutput(true, true, str.str());
        }
    }

    void DynamicIPNode::propertyChanged(const Property* p)
    {
        if (p == m_glslCurrent)
            rebuild();

        if (m_parameters->hasProperty(p))
        {
            //
            //  Parameters which change in the dynamic node are copied
            //  onto the the internal node.
            //

            if (Property* ip = m_internalParameters->find(p->name()))
            {
                ip->copy(p);
            }
        }

        IPNode::propertyChanged(p);
    }

    void DynamicIPNode::readCompleted(const string& p, unsigned int v)
    {
        //
        //  Don't assume the valid code really is valid -- recompile it from
        //  scratch
        //

        m_glslCurrent->copy(m_glslValid);
        rebuild();

        GroupIPNode::readCompleted(p, v);

        //
        //  Check for UI error
        //

        if (m_outputError->front())
        {
            cout << "ERROR: " << m_outputMessage->front() << endl;
        }
    }

    string DynamicIPNode::stringValue(const string& name) const
    {
        return propertyValue<StringProperty>(name, "");
    }

    IPNode::StringProperty* DynamicIPNode::stringProp(const string& name)
    {
        return property<StringProperty>(name);
    }

    void DynamicIPNode::setStringValue(const string& name, const string& value,
                                       bool withNotification)
    {
        if (StringProperty* sp = property<StringProperty>(name))
        {
            sp->resize(1);
            sp->front() = value;
        }
    }

    int DynamicIPNode::intValue(const string& name) const
    {
        return propertyValue<IntProperty>(name, 0);
    }

    IPNode::IntProperty* DynamicIPNode::intProp(const string& name)
    {
        return property<IntProperty>(name);
    }

    void DynamicIPNode::setIntValue(const string& name, int value,
                                    bool withNotification)
    {
        if (IntProperty* p = property<IntProperty>(name))
        {
            p->resize(1);
            p->front() = value;
        }
    }

    bool DynamicIPNode::outputError() const
    {
        return property<IntProperty>("output.error")->front() != 0;
    }

    bool DynamicIPNode::outputValid() const
    {
        return property<IntProperty>("output.valid")->front() != 0;
    }

    string DynamicIPNode::outputMessage() const
    {
        return property<StringProperty>("output.message")->front();
    }

    string DynamicIPNode::outputFunctionName() const
    {
        return m_outputMain->front();
    }

    string DynamicIPNode::outputFunctionCallName() const
    {
        return m_outputCallMain->front();
    }

} // namespace IPCore
