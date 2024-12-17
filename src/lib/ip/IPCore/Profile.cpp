//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/Profile.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/GTOWriter.h>
#include <TwkUtil/PathConform.h>
#include <boost/filesystem.hpp>
#include <TwkApp/Bundle.h>
#include <sstream>

namespace IPCore
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkContainer;
    using namespace boost;
    using namespace boost::filesystem;
    using namespace TwkApp;

    //
    //  The 'tag' is an arbitary string that can be used for categorizing
    //  profiles in the GUI and eliminanting name conflicts.  For example, all
    //  DisplayPipelineGroup profiles use the tag 'display'.  It's up to the
    //  writer of the profile file to put the tag in the filename.
    //

    static vector<path> collectProfilesFromPath()
    {
        Bundle* bundle = Bundle::mainBundle();
        Bundle::PathVector paths = bundle->pluginPath("Profiles");
        vector<path> fspaths;
        for (size_t i = 0; i < paths.size(); i++)
        {
            path dir = paths[i];

            try
            {
                if (exists(dir) && is_directory(dir))
                {
                    for (directory_iterator q(dir); q != directory_iterator();
                         ++q)
                    {
                        path p = q->path();

                        if (p.extension() == ".profile")
                        {
                            fspaths.push_back(p);
                        }
                    }
                }
            }
            catch (filesystem_error& er)
            {
                cout << "WARNING: cannot read from \"" << dir.string() << "\""
                     << endl;
                continue;
            }
        }
        return fspaths;
    }

    string profileMatchingNameInPath(const string& name, const string& tag,
                                     IPGraph* graph)
    {
        vector<path> paths = collectProfilesFromPath();

        path n = name;
        if (n.is_absolute() && exists(n))
            return name;

        for (int i = 0; i < paths.size(); i++)
        {
            path p = paths[i];
            path st = p.stem();

            if (!tag.empty() && st.extension() == string(".") + tag)
            {
                st = st.stem();
            }
            if (st == name)
            {
                return p.string();
            }
        }

        return "";
    }

    void profilesInPath(ProfileVector& profiles, const string& tag,
                        IPGraph* graph)
    {
        profiles.clear();
        vector<path> paths = collectProfilesFromPath();
        for (int i = 0; i < paths.size(); i++)
        {
            path p = paths[i];
            if (tag != "" && p.stem().extension() != string(".") + tag)
            {
                continue;
            }
            profiles.push_back(new Profile(p.string(), graph));
        }
    }

    Profile::Profile(const std::string& infilename, IPGraph* graph)
        : m_reader(0)
        , m_root(0)
        , m_header(0)
        , m_filename(pathConform(infilename))
        , m_graph(graph)
    {
        path p(infilename);
        m_name = p.filename().replace_extension().string();
        m_tag = path(m_name).extension().string();

        if (!m_tag.empty())
        {
            m_tag = m_tag.substr(1);
            m_name = path(m_name).replace_extension().string();
        }
    }

    Profile::~Profile()
    {
        if (m_reader)
        {
            m_reader->deleteAll();
            delete m_reader;
        }
    }

    void Profile::load()
    {
        const string filename = pathConform(m_filename);

        m_reader = new GTOReader;
        m_reader->read(filename.c_str());

        m_header = m_reader->findByProtocol("Profile");

        if (!m_header)
        {
            TWK_THROW_EXC_STREAM("File " << m_filename
                                         << " contains no profile");
        }

        const string rootName =
            m_header->propertyValue<StringProperty>("root.name", "");
        m_root = m_reader->findByName(rootName);

        if (!m_root)
        {
            TWK_THROW_EXC_STREAM("File " << m_filename << " missing root");
        }
    }

    namespace
    {

        typedef TwkContainer::StringPairProperty::container_type
            ConnectionContainer;

        //
        //  PROFILE HELPHER FUNCTIONS
        //

        void profileNodeInputs(const GTOReader& reader,
                               PropertyContainer* profileNode,
                               const ConnectionContainer& connections,
                               vector<PropertyContainer*>& containers)
        {
            containers.clear();

            for (size_t i = 0; i < connections.size(); i++)
            {
                //
                //  Evaluation connections are stored in data flow direction:
                //  lhs is the input node. Order is assumed to be maintained
                //  in the connection list.
                //

                if (connections[i].second == profileNode->name())
                {
                    containers.push_back(
                        reader.findByName(connections[i].first));
                }
            }
        }

        void applyProfileGraph(const GTOReader& reader, IPNode* node,
                               PropertyContainer* profileNode,
                               const ConnectionContainer& connections)
        {
            //
            //  Apply the state in profileNode to node (actual node in graph).
            //  If node is a group node recursively apply on all members.
            //  Recursively apply on inputs.
            //

            if (profileNode->protocol() != node->protocol())
                return;

#if 0
    cout << "applyProfileGraph: " 
         << node->name() << ":" << node->protocol()
         << " <- "
         << profileNode->name()
         << endl;
#endif

            vector<PropertyContainer*> profileInputs;
            profileNodeInputs(reader, profileNode, connections, profileInputs);

            //
            //  Copy state in profileNode to node. This will not remove state
            //  from node that's not shared with profileNode. Since this is
            //  basically a file read don't worry about propagating property
            //  changes just call the node's readCompleted(). That function
            //  has to perform any node reconfiguration since session files
            //  also avoid property changed notification. (i.e we don't need
            //  to use PropertyEditor class)
            //

            //
            //  XXX Note: Calling copy and readCompleted like this assumes
            //  that readCompleted does not depend of the property values in
            //  any other nodes since they may not bet set yet.
            //

            node->copy(profileNode);
            node->readCompleted(node->protocol(), node->protocolVersion());

            //
            //  After the state has been set, the node may have reconfigured
            //  itself. So now the members of the profileNode need to be
            //  matched with the members of the actual node.
            //

            const string rootName = profileNode->propertyValue<StringProperty>(
                "evaluation.root", "");

            if (rootName != "")
            {
                if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(node))
                {
                    if (PropertyContainer* rootContainer =
                            reader.findByName(rootName))
                    {
                        const ConnectionContainer& cons =
                            profileNode->propertyContainer<StringPairProperty>(
                                "evaluation.connections");

                        //
                        //  Only recursively apply if there's actually a
                        //  subgraph associated with the profileNode.
                        //

                        IPNode* groupRoot = group->rootNode();
                        applyProfileGraph(reader, groupRoot, rootContainer,
                                          cons);
                    }
                }
            }

            //
            //  If this profile node has any inputs recursively follow them. Its
            //  somewhat questionable to attempt this when there isn't an exact
            //  match between the nodes number of inputs and protocols.
            //

            if (profileInputs.size() == node->inputs().size())
            {
                const IPNode::IPNodes& nodeInputs = node->inputs();

                for (size_t i = 0; i < profileInputs.size(); i++)
                {
                    if (i < nodeInputs.size())
                    {
                        PropertyContainer* input = profileInputs[i];
                        if (input)
                            applyProfileGraph(reader, nodeInputs[i], input,
                                              connections);
                    }
                }
            }
        }

    } // namespace

    void Profile::apply(IPNode* node)
    {
        //
        //  In case the Profile has not been previosly loaded.
        //
        if (!m_root)
            load();

        //
        //  NOTE: the name so the nodes in the profile will not in general
        //  match those in the actual graph. So to apply the profile you
        //  need to match the graph structure and types of the nodes
        //

        if (node->graph() != m_graph)
        {
            TWK_THROW_EXC_STREAM(
                "Profile: node graph does not match profile graph");
        }

        //
        //  Forbid, for now, applying a profile to a different type of group
        //  node from the type of the node from which the profile was saved.
        //

        if (m_root->protocol() != node->protocol())
        {
            TWK_THROW_EXC_STREAM("Profile: cannot apply "
                                 << m_root->protocol() << " profile to "
                                 << node->protocol() << " node.");
        }

        IPGraph::GraphEdit edit(*node->graph());
        const StringPairProperty::container_type emptyConnections;

        applyProfileGraph(*m_reader, node, m_root, emptyConnections);

        PropertyInfo* noSave = new PropertyInfo(PropertyInfo::NotPersistent
                                                | PropertyInfo::NotCopyable
                                                | PropertyInfo::OutputOnly);

        node->declareProperty<StringProperty>("profile.name", name(), noSave);
        node->declareProperty<StringProperty>("profile.file", fileName(),
                                              noSave);
    }

    string Profile::comment() const
    {
        return m_header
                   ? m_header->propertyValue<StringProperty>("root.comment", "")
                   : "NOT LOADED";
    }

    string Profile::structureDescription() const
    {
        if (!m_header)
            return "NOT LOADED";

        //
        //  For each node's props report the value if it differs from the
        //  default
        //

        const GTOReader::Containers& containers = m_reader->containersRead();

        ostringstream str;

        for (size_t i = 0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            const string& protocol = pc->protocol();

            if (protocol == "Profile")
                continue;

            if (IPNode* genericNode = m_graph->newNode(protocol, "_x"))
            {
                PropertyContainer* diffpc = pc->shallowDiffCopy(genericNode);
                m_graph->deleteNode(genericNode);

                PropertyContainer::NamedPropertyMap pmap;
                diffpc->propertiesAsMap(pmap);

                for (PropertyContainer::NamedPropertyMap::const_iterator q =
                         pmap.begin();
                     q != pmap.end(); ++q)
                {
                    const string& name = q->first;
                    Property* p = q->second;

                    if (name == "evaluation.connections"
                        || name == "evaluation.root" || name == "input.index"
                        || name == "membership.contains")
                    {
                        continue;
                    }

                    str << protocol << "." << name << " = "
                        << p->valueAsString() << endl;
                }
            }
        }

        return str.str();
    }

} // namespace IPCore
