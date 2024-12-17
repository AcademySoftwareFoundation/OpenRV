//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__NodeDefinition__h__
#define __IPCore__NodeDefinition__h__
#include <iostream>
#include <TwkContainer/PropertyContainer.h>
#include <map>
#include <string>
#include <vector>

namespace IPCore
{
    class IPGraph;
    class GroupIPNode;
    class IPNode;

    namespace Shader
    {
        class Function;
    }

    /// NodeDefinition holds the state for a node loaded from filesystem or
    /// network

    ///
    /// NodeDefinition is a PropertyContainer, but it is NOT an
    /// IPNode. This class holds data needed by NodeManager to create
    /// instances of user defined nodes.
    ///
    /// A NodeDefinition can be serialized as a GTO object. It can
    /// therefor be saved as either a system wide resource or as a member
    /// of a session file.
    ///
    /// Important properties on the node definition:
    ///
    ///     string node.author              description
    ///     string node.company             description
    ///     string node.comment             description
    ///     string node.evaluationType      determines IPInstanceNode
    ///     sub-classes int    node.userVisible         1 if user can make one
    ///     of these from UI int    function.fetches         number of textel
    ///     fetches made by shader
    ///
    ///     string documentation.summary    single line doc string
    ///     string documentation.html       html documentation blob or URL
    ///     byte[] icon.RGBA                8 bit RGBA icon (power-of-two sized)
    ///
    ///     int    render.intermediate      1 if intermediate render
    ///     string function.type            function type (merge, inline, etc)
    ///
    ///     int    usage.fetches            estimated number of fetches
    ///

    class NodeDefinition : public TwkContainer::PropertyContainer
    {
    public:
        //
        //  Types
        //

        enum NodeType
        {
            UnknownNodeType,
            AtomicNodeType,
            GroupNodeType,
            InternalNodeType
        };

        typedef IPNode* (*NodeConstructor)(const std::string&,
                                           const NodeDefinition*, IPGraph*,
                                           GroupIPNode*);

        typedef std::map<std::string, NodeConstructor> NodeConstructorMap;
        typedef TwkContainer::ByteProperty::container_type ByteVector;
        typedef std::vector<std::string> StringVector;
        typedef TwkContainer::PropertyContainer PropertyContainer;
        typedef TwkContainer::Property Property;
        typedef TwkContainer::Component Component;
        typedef TwkContainer::ByteProperty ByteProperty;
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::StringProperty StringProperty;

        //
        //  Constructor for serialized PropertyContainer (GLSL, etc)
        //

        NodeDefinition(const PropertyContainer* serializedDefinition);

        //
        //  Constructor for internal code
        //

        NodeDefinition(const std::string& typeName, unsigned int version,
                       bool isGroup, const std::string& defaultName,
                       NodeConstructor, const std::string& summary,
                       const std::string& htmlDocs, const ByteVector iconRGBA,
                       bool userVisible = true);

        virtual ~NodeDefinition();

        //
        //  Create a node of this type
        //

        IPNode* newNode(const std::string& name, IPGraph* graph,
                        GroupIPNode* group) const;

        template <typename T>
        T* newNodeOfType(const std::string& name, IPGraph* graph,
                         GroupIPNode* group) const
        {
            return dynamic_cast<T*>(newNode(name, graph, group));
        }

        //
        //  Accessor functions: these are just wrappers around
        //  setProperty<> and property<> for convenience.
        //

        void setString(const std::string& fullname, const std::string&);
        void setInt(const std::string& fullname, int);

        std::string stringValue(const std::string& fullname,
                                const std::string& defaultValue = "") const;
        std::vector<std::string>
        stringArrayValue(const std::string& fullname) const;
        int intValue(const std::string& fullname, int defaultValue = 0) const;

        bool userVisible() const { return intValue("node.userVisible") == 1; }

        bool isGroup() const { return intValue("node.isGroup") == 1; }

        //
        //  Convert saved state into actual verified internal state. In
        //  the case of an AtomicNodeType this only means construct a
        //  function. Language specific syntax (GLSL, OpenCL, CUDA) is not
        //  verified until compilation occurs.
        //

        bool reify();

        //
        //  This is only non-zero in the case of an AtomicNodeType
        //

        const Shader::Function* function() const { return m_function; }

        //
        //  Reload shader source and rebuild function.
        //

        bool rebuildFunction();

    private:
        bool buildFunction();
        Shader::Function* buildFunctionCore();

    private:
        NodeType m_type;
        Shader::Function* m_function;
        NodeConstructor m_constructor;
    };

} // namespace IPCore

#endif // __IPCore__NodeDefinition__h__
