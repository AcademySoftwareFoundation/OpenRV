//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/NodeDefinition.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/IPInstanceNode.h>
#include <IPCore/IPInstanceGroupNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/TransitionIPInstanceNode.h>
#include <IPCore/ColorIPInstanceNode.h>
#include <IPCore/FilterIPInstanceNode.h>
#include <IPCore/StackIPInstanceNode.h>
#include <IPCore/CombineIPInstanceNode.h>
#include <TwkUtil/PathConform.h>
#include <TwkDeploy/Deploy.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

namespace IPCore
{
    using namespace std;
    using namespace boost;

    NodeDefinition::NodeDefinition(const string& typeName, unsigned int version,
                                   bool isGroup, const string& defaultName,
                                   NodeConstructor constructor,
                                   const string& summary, const string& html,
                                   ByteVector iconRGBA, bool userVisible)
        : TwkContainer::PropertyContainer()
        , m_constructor(constructor)
        , m_type(InternalNodeType)
        , m_function(0)
    {
        setName(typeName);
        setProtocol("IPNodeDefinition");
        setProtocolVersion(1);
        declareProperty<StringProperty>("node.name", typeName);
        declareProperty<IntProperty>("node.version", int(version));
        declareProperty<IntProperty>("node.isGroup", int(isGroup ? 1 : 0));
        declareProperty<StringProperty>("node.defaultName", defaultName);
        declareProperty<StringProperty>("node.evaluationType",
                                        isGroup ? "group" : "merge");
        declareProperty<StringProperty>("node.author", "internal");
        declareProperty<StringProperty>("node.company", "");
        declareProperty<StringProperty>("node.comment", "");
        declareProperty<StringProperty>("node.date", "");
        declareProperty<IntProperty>("node.userVisible", userVisible ? 1 : 0);
        declareProperty<IntProperty>("function.fetches", 0);
        declareProperty<StringProperty>("documentation.summary", summary);
        declareProperty<StringProperty>("documentation.html", html);
        declareProperty<ByteProperty>("icon.RGBA", iconRGBA);
    }

    NodeDefinition::NodeDefinition(const PropertyContainer* pc)
        : TwkContainer::PropertyContainer()
        , m_constructor(0)
        , m_function(0)
        , m_type(UnknownNodeType)
    {
        string objName = pc->name();
        string protocol = pc->protocol();
        int version = pc->protocolVersion();

        if (protocol == "IPNodeDefinition")
        {
            copyNameAndProtocol(pc);
            m_type = AtomicNodeType;
            copy(pc);

            if (version != 1)
            {
                cout << "WARNING: node definition version " << version
                     << " is newer than version 1 for " << objName << endl;
            }
        }
        else if (protocol == "Dynamic")
        {
            objName = pc->propertyValue<StringProperty>(
                "node.name", "AGoodNameForANodeType");
            setName(objName);
            setProtocol("IPNodeDefinition");
            setProtocolVersion(1);
            m_type = AtomicNodeType;
            copy(pc);
        }
        else if (protocol == "IPGroupNodeDefinition")
        {
            copyNameAndProtocol(pc);
            copy(pc);

            m_type = GroupNodeType;

            if (version != 1)
            {
                cout << "WARNING: group node definition version " << version
                     << " is newer than version 1 for " << objName << endl;
            }
        }
        else
        {
            abort();
            return;
        }

        declareProperty<StringProperty>("node.name", objName, 0, false);
        declareProperty<IntProperty>("node.version", 1, 0, false);
        declareProperty<IntProperty>("node.isGroup", 0, 0, false);
        declareProperty<StringProperty>("node.defaultName", "node", 0, false);
        declareProperty<StringProperty>("node.evaluationType", "merge", 0,
                                        false);
        declareProperty<StringProperty>("node.author", "", 0, false);
        declareProperty<StringProperty>("node.company", "", 0, false);
        declareProperty<StringProperty>("node.comment", "", 0, false);
        declareProperty<StringProperty>("node.date", "", 0, false);
        declareProperty<IntProperty>("node.userVisible", 1, 0, false);
        declareProperty<StringProperty>("documentation.summary", "", 0, false);
        declareProperty<StringProperty>("documentation.html", "", 0, false);
        declareProperty<ByteProperty>("icon.RGBA", 0, false);

        switch (m_type)
        {
        case AtomicNodeType:
            declareProperty<IntProperty>("render.intermediate", 0, 0, false);
            declareProperty<StringProperty>("function.type", "", 0, false);
            declareProperty<StringProperty>("function.name", "main", 0, false);
            break;

        case GroupNodeType:
            break;

        default:
            break;
        }
    }

    NodeDefinition::~NodeDefinition()
    {
        m_function->retire();
        m_function = 0;
    }

    IPNode* NodeDefinition::newNode(const std::string& name, IPGraph* graph,
                                    GroupIPNode* group) const
    {
        IPNode* node = 0;

        //
        //  If name is "" use the defaultName in the node
        //  definition. Also, make sure the name is unique for the passed
        //  in graph.
        //

        string nodeName = name == "" ? stringValue("node.defaultName") : name;
        nodeName = graph->uniqueName(nodeName);

        switch (m_type)
        {
        case AtomicNodeType:
        {
            string ntype = stringValue("node.evaluationType");

            if (ntype == "transition")
            {
                node =
                    new TransitionIPInstanceNode(nodeName, this, graph, group);
            }
            else if (ntype == "color")
            {
                node = new ColorIPInstanceNode(nodeName, this, graph, group);
            }
            else if (ntype == "filter")
            {
                node = new FilterIPInstanceNode(nodeName, this, graph, group);
            }
            else if (ntype == "combine")
            {
                node = new CombineIPInstanceNode(nodeName, this, graph, group);
            }
            else if (ntype == "merge" || ntype == "stack")
            {
                node = new StackIPInstanceNode(nodeName, this, graph, group);
            }
            else
            {
                TWK_THROW_EXC_STREAM("unknown node.evaluationType \""
                                     << ntype << "\" for definition "
                                     << this->name());
            }
        }
        break;

        case GroupNodeType:
            node = new IPInstanceGroupNode(nodeName, this, graph, group);
            break;

        case InternalNodeType:
            node = (*m_constructor)(nodeName, this, graph, group);
            break;

        default:
            break;
        }

        //
        //  If the incoming node definition has a parameters component
        //  copy it over to this one.
        //

        if (const Component* c = component("parameters"))
        {
            if (Component* parameters = node->component("parameters"))
            {
                parameters->copy(c);
            }
            else
            {
                Component* parameters2 = new Component("parameters");
                parameters2->copy(c);
                node->add(parameters2);
            }
        }

        return node;
    }

    string NodeDefinition::stringValue(const string& name,
                                       const string& defaultValue) const
    {
        return propertyValue<StringProperty>(name, defaultValue);
    }

    vector<string> NodeDefinition::stringArrayValue(const string& name) const
    {
        if (const StringProperty* sp = property<StringProperty>(name))
        {
            return sp->valueContainer();
        }

        return vector<string>();
    }

    void NodeDefinition::setString(const string& name, const string& value)
    {
        setProperty<StringProperty>(name, value);
    }

    int NodeDefinition::intValue(const string& name, int defaultValue) const
    {
        return propertyValue<IntProperty>(name, defaultValue);
    }

    void NodeDefinition::setInt(const string& name, int value)
    {
        setProperty<IntProperty>(name, value);
    }

    bool NodeDefinition::reify()
    {
        switch (m_type)
        {
        case UnknownNodeType:
            TWK_THROW_EXC_STREAM("unknown definition prototype");
            return false;
        case AtomicNodeType:
            return buildFunction();
        case GroupNodeType:
            break;
        case InternalNodeType:
            break;
        }
        return true;
    }

#define CURRENT_SIGNATURE_VERSION 1

    namespace
    {

        string readURL(const string& url, const string& origin)
        {
            //
            //  Scarf the file. Dog help us on windows.
            //

            string fileName = url.substr(7, url.size() - 7);
            string source;

            regex varRE("\\$\\{(\\w+)\\}");
            smatch m;

            //
            //  Check for variable subsitution. Any defined environment
            //  variable will replaced with the exception of ${HERE} which
            //  indicates the directory in which this file was found. So use
            //  of "HERE" as an env var may not get the expected results.
            //

            while (regex_search(fileName, m, varRE))
            {
                string varName = m[1];

                if (varName == "HERE")
                {
                    boost::filesystem::path p = origin;
                    fileName = regex_replace(fileName, varRE,
                                             p.parent_path().string());
                }
                else if (const char* value = getenv(varName.c_str()))
                {
                    fileName = regex_replace(fileName, varRE, string(value));
                }
            }

            //
            //  Make sure the path is ok for windows and then open the file
            //  and use the rdbuf trick to dump its contents.
            //

            fileName = TwkUtil::pathConform(fileName);
            ifstream in(fileName.c_str());

            if (in.good())
            {
                stringstream buffer;
                buffer << in.rdbuf();
                source = buffer.str();
            }
            else
            {
                TWK_THROW_EXC_STREAM("failed to load glsl file: " << fileName);
            }

            //
            //  Check for BOM and remove it if exists. This allows mac and
            //  linux to read windows text files. This could be better done by
            //  reading only the first three characters from the file then
            //  continuing, but whatever. This *should* be a rare case.
            //

            if (source.size() > 3 && source[0] == string::value_type(0xEF)
                && source[1] == string::value_type(0xBB)
                && source[2] == string::value_type(0xBF))
            {
                source.erase(0, 3);
                // cout << "WARINING: file " << fileName << " has a BOM --
                // removed it" << endl;
            }

            return source;
        }

    } // namespace

    Shader::Function* NodeDefinition::buildFunctionCore()
    {
        string origin = stringValue("node.origin");
        string evalType = stringValue("node.evaluationType");
        string source = stringValue("function.glsl");
        string ftype = stringValue("function.type");
        string fname = stringValue("function.name");
        string docs = name();

        if (fname == "")
            fname = "main";

        //
        //  If the source code is a file URL than get the source from the
        //  URL
        //

        if (source.size() > 7 && source.substr(0, 7) == "file://")
        {
            source = readURL(source, origin);
        }

        //
        //  If the definition is part of a dynamic node, uniqify its
        //  source code by adding a comment with the NodeDefinition's
        //  pointer value. This way two default dynamic nodes will not
        //  cause a hash conflict.
        //

        if (origin == "dynamic")
        {
            ostringstream str;
            str << endl << "//NODE:" << this << endl;
            source += str.str();
        }

        //
        //  If ftype was not declared the Function will infer its
        //  type. MorphologicalFilter and LinearColor types can not be
        //  infered; these will result in Filter and Color respectively.
        //

        Shader::Function::Type type = Shader::Function::UndecidedType;

        if (ftype == "merge")
            type = Shader::Function::Merge;
        else if (ftype == "filter")
            type = Shader::Function::Filter;
        else if (ftype == "morphological")
            type = Shader::Function::MorphologicalFilter;
        else if (ftype == "color")
            type = Shader::Function::Color;
        else if (ftype == "linear color")
            type = Shader::Function::LinearColor; // do we need this?
        else if (ftype == "source")
            type = Shader::Function::Source;

        Shader::Function* f = NULL;

        try
        {
            f = new Shader::Function(fname, source, type,
                                     intValue("function.fetches"), docs);

            if (type == Shader::Function::UndecidedType)
            {
                switch (f->type())
                {
                case Shader::Function::Source:
                    ftype = "source";
                    break;
                default:
                case Shader::Function::Color:
                    ftype = "color";
                    break;
                case Shader::Function::LinearColor:
                    ftype = "linear color";
                    break;
                case Shader::Function::Merge:
                    ftype = "merge";
                    break;
                case Shader::Function::Filter:
                    ftype = "filter";
                    break;
                case Shader::Function::MorphologicalFilter:
                    ftype = "morphological";
                    break;
                }

                setString("function.type", ftype);
            }

            string eguess = "";

            switch (f->type())
            {
            case Shader::Function::Source:
                eguess = "generator";
                break;
            default:
            case Shader::Function::Color:
                eguess = "color";
                break;
            case Shader::Function::LinearColor:
                eguess = "color";
                break;
            case Shader::Function::Merge:
                eguess = "merge";
                break;
            case Shader::Function::Filter:
            case Shader::Function::MorphologicalFilter:
                eguess = "filter";
                break;
            }

#if 0
        if (eguess != evalType)
        {
            cout << "WARNING: node type " << name()
                 << " marked as " << evalType
                 << " evaluation type, but appears to be " << eguess
                 << endl;
        }
#endif

            if (evalType == "combine")
            {
                // nothing
            }
            else if (evalType != "transition" && evalType != "merge"
                     && f->imageParameters().size() > 1)
            {
                setString("node.evaluationType", "merge");
            }
            else if (eguess != evalType
                     && (((evalType == "color" || evalType == "filter")
                          && eguess == "merge")
                         || (evalType == "filter" && eguess != "filter")
                         || (evalType == "transition" && eguess == "generator")
                         || (evalType == "color" && eguess == "filter")))
            {
                setString("node.evaluationType", eguess);
            }
        }
        catch (std::exception& exc)
        {
            ostringstream str;
            str << "ERROR: " << exc.what();
            declareProperty<StringProperty>("error.message", str.str(), 0,
                                            true);
            cout << "ERROR: failed to build/parse function " << fname << endl;
            f = NULL;
        }
        catch (...)
        {
            ostringstream str;
            str << "ERROR: failed to build/parse function " << fname << endl;
            declareProperty<StringProperty>("error.message", str.str(), 0,
                                            true);
            cout << "ERROR: failed to build/parse function " << fname << endl;
            f = NULL;
        }

        return f;
    }

    bool NodeDefinition::buildFunction()
    {
        m_function = buildFunctionCore();

        return (m_function != NULL);
    }

    bool NodeDefinition::rebuildFunction()
    {
        Shader::Function* f = buildFunctionCore();
        if (!f)
            return false;

        if (!m_function->equivalentInterface(f))
        {
            cerr << "ERROR: rebuilt shader for '" << name()
                 << "' has different interface." << endl;
            return false;
        }

        m_function->retire();
        m_function = f;

        return true;
    }

} // namespace IPCore
