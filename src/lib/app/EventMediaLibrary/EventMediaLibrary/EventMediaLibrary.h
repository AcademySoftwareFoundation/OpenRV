//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __EventMediaLibrary__EventMediaLibrary__h__
#define __EventMediaLibrary__EventMediaLibrary__h__

#include <TwkMediaLibrary/Library.h>
#include <iostream>

namespace TwkMediaLibrary {
class EventMediaLibrary;

enum EventNodeType
{
    EventNoType = 0,
    EventRootType,
    EventMediaType
};

class EventNode : public Node
{
  public:
    using EventNodeVector = std::vector<EventNode*>;
    using NodeAPIVector = std::vector<NodeAPI*>;

    const Node* parent() const override;
    size_t numChildren() const override;
    const Node* child(size_t index) const override;
    const Library* library() const override { return m_library; }

    bool isValid() const override;
    std::string name() const override;
    std::string typeName() const override;
    std::string description() const override;

    InternalPath path() const override;
    InternalPath mediaPath() const override;

    LibraryURL url() const override;
    LibraryURL persistentURL() const override;
    URL mediaURL() const override;
    URL thumbnailImageURL() const override;

    MimeType mediaMimeType() const override;

    void setMediaURL(const URL& url) { m_mediaURL = url; }

  protected:
    explicit EventNode(EventMediaLibrary*, EventNode* parent=nullptr);
    virtual ~EventNode();

    void addChild(EventNode*);
    void setName(const std::string&) override;
    void setParent(const Node*) override;

    const NodeAPI* nodeAPI() const { return nullptr; }
    const EventMediaLibrary* lib() const;
    EventMediaLibrary* lib();

    friend class EventMediaLibrary;

  private:
    EventNodeType    m_type;
    std::string      m_name;
    EventNode*       m_parent;
    EventNodeVector  m_children;
    NodeAPIVector    m_apiVector;
    URL              m_mediaURL;
};

class EventRootNode;
class EventMediaNode;

class EventMediaLibrary : public Library
{
  public:
    using StreamingURLNodeMap = std::map<URL, EventMediaNode*>;

    explicit EventMediaLibrary(const std::string& appName);
    virtual ~EventMediaLibrary();

    bool isReadable() const override;
    bool isWriteable() const override;
    bool hasWritableProperties() const override;
    bool hasImport() const override;
    bool hasExport() const override;

    size_t numNodeTypeNames() const override;
    std::string nodeTypeName(size_t) const override;

    bool isLibraryMediaURL(const URL&) const override;

    const Node* rootNode() const override;
    const Node* nodeFromInternalPath(const InternalPath&) const override;
    const Node* nodeFromLibraryURL(const LibraryURL&) const override;
    void deleteNode(const Node*) override;
    void reorderNode(const Node*, size_t newIndex) override;

    NodeVector associatedNodes(const URL& url) const override;
    const NodeAPI* nodeAPI(const Node*) const override;

  private:
    EventRootNode*              m_root;
    mutable StreamingURLNodeMap m_streamingMap;
};

} // TwkMediaLibrary

#endif // __EventMediaLibrary__EventMediaLibrary__h__
