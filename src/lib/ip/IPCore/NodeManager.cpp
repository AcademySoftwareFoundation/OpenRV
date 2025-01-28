//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/NodeManager.h>
#include <IPCore/NodeManager.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/Exception.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/GTOWriter.h>
#include <iterator>
#include <algorithm>
#include <exception>
#include <TwkUtil/PathConform.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/regex.hpp>

#ifdef PLATFORM_WINDOWS
#define SEP ";"
#else
#define SEP ":"
#endif

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace boost;
    using namespace boost::filesystem;
    using namespace TwkUtil;

    bool NodeManager::m_debug = false;

    NodeManager::NodeManager() {}

    NodeManager::~NodeManager() {}

    void NodeManager::loadDefinitionsAlongPathVar(const string& paths)
    {
        vector<string> parts;
        algorithm::split(parts, paths, is_any_of(string(SEP)),
                         token_compress_on);

        for (size_t i = 0; i < parts.size(); i++)
        {
            path nodedir(parts[i]);
            if (m_debug)
                cout << "INFO: searching for nodes in " << parts[i] << endl;

            try
            {
                if (exists(nodedir) && is_directory(nodedir))
                {
                    for (directory_iterator di(nodedir);
                         di != directory_iterator(); ++di)
                    {
                        path p = di->path();

                        if (p.has_extension())
                        {
                            path ext = p.extension();

                            if (ext == ".gto" || ext == ".GTO")
                            {
                                try
                                {
                                    loadDefinitions(p.string());
                                }
                                catch (std::exception& e)
                                {
                                    cout << "WARNING: caught exception reading "
                                         << p.string() << ": " << e.what()
                                         << endl;
                                }
                            }
                        }
                    }
                }
            }
            catch (filesystem_error& ex)
            {
                cerr << "ERROR: Cannot read from \"" << nodedir.string() << "\""
                     << endl;
                continue;
            }
        }
    }

    void NodeManager::loadDefinitions(const string& infile)
    {
        try
        {
            GTOReader reader;
            GTOReader::Containers containers = reader.read(infile.c_str());

            Property::Info* notPersistent = new Property::Info();
            notPersistent->setPersistent(false);

            if (m_debug)
                cout << "INFO: read node definition file " << infile << endl;

            for (size_t i = 0; i < containers.size(); i++)
            {
                PropertyContainer* pc = containers[i];
                pc->declareProperty<StringProperty>("node.origin", infile,
                                                    notPersistent);
                addDefinition(new NodeDefinition(pc));
            }
        }
        catch (...)
        {
            cout << "ERROR: failed to load definition file " << infile << endl;
        }
    }

    void NodeManager::addDefinition(NodeDefinition* def)
    {
        try
        {
            if (def->reify())
            {
                if (m_definitionMap.count(def->name()) > 0)
                {
                    //
                    //  Stash definitions that are being replaced. You can't
                    //  delete these because some node might be using it.
                    //

                    m_retiredDefinitions.push_back(
                        m_definitionMap[def->name()]);
                }

                m_definitionMap[def->name()] = def;
                if (m_debug)
                    cout << "INFO: added node type " << def->name() << endl;
            }
        }
        catch (ShaderSignatureExc& e)
        {
            if (m_debug)
                cout << "WARNING: skipping unsigned shader: " << def->name()
                     << ": " << e.what() << endl;
        }
        catch (std::exception& e)
        {
            cerr << "ERROR: failed to add node definition: " << e.what()
                 << endl;
            delete def;
            def = NULL;
            throw;
        }
    }

    const NodeDefinition* NodeManager::definition(const string& typeName) const
    {
        NodeDefinitionMap::const_iterator i = m_definitionMap.find(typeName);
        return i == m_definitionMap.end() ? 0 : (*i).second;
    }

    IPNode* NodeManager::newNode(const string& typeName, const string& nodeName,
                                 IPGraph* graph, GroupIPNode* group) const
    {
        if (const NodeDefinition* def = definition(typeName))
        {
            return def->newNode(nodeName, graph, group);
        }
        else
        {
            return 0;
        }
    }

    bool NodeManager::updateDefinition(const string& typeName)
    {
        NodeDefinitionMap::const_iterator i = m_definitionMap.find(typeName);

        if (i != m_definitionMap.end())
        {
            return (*i).second->rebuildFunction();
        }

        return false;
    }

    void NodeManager::writeAllDefinitions(const string& filename,
                                          bool inlineSourceCode) const
    {
        NodeDefinitionVector defs;
        const NodeManager::NodeDefinitionMap& map = m_definitionMap;

        for (NodeManager::NodeDefinitionMap::const_iterator i = map.begin();
             i != map.end(); ++i)
        {
            const NodeDefinition* definition = (*i).second;

            if (definition->function()
                && definition->function()->originalSource() != "")
            {
                defs.push_back(definition);
            }
        }

        writeDefinitions(filename, defs, inlineSourceCode);
    }

    void NodeManager::writeDefinitions(const string& infilename,
                                       const NodeDefinitionVector& defs,
                                       bool inlineSourceCode) const
    {
        vector<PropertyContainer*> tempContainers;
        GTOWriter::ObjectVector objects;
        GTOWriter writer;

        for (size_t i = 0; i < defs.size(); i++)
        {
            const NodeDefinition* definition = defs[i];

            if (!definition->function()
                || definition->function()->originalSource() == "")
            {
                continue;
            }

            PropertyContainer* tempPC = definition->copy();

            if (inlineSourceCode)
            {
                tempPC->setProperty<StringProperty>(
                    "function.glsl", definition->function()->originalSource());
            }

            tempContainers.push_back(tempPC);
            objects.push_back(GTOWriter::Object(tempPC));
        }

        if (objects.size() == 0)
            TWK_THROW_EXC_STREAM("No writable nodes.");

        string filename = pathConform(infilename);
        writer.write(filename.c_str(), objects, Gto::Writer::TextGTO);

        for (size_t i = 0; i < tempContainers.size(); i++)
            delete tempContainers[i];
    }

} // namespace IPCore
