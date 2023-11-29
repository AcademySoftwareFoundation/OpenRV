//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved. 
//  SPDX-License-Identifier: Apache-2.0
//
#include <EventMediaLibrary/EventMediaLibrary.h>
#include <RvApp/RvSession.h>
#include <QtCore/QUrl>

namespace TwkMediaLibrary {

//----------------------------------------------------------------------

class EventMediaAPI;

class EventMediaNode : public EventNode, public MediaAPI
{
  public:
    EventMediaNode(EventMediaLibrary* lib,
                const URL& url,
                EventNode* parent)
        : EventNode(lib, parent),
          MediaAPI(),
          m_url(url) {}
    virtual ~EventMediaNode() {}
    const NodeAPI* nodeAPI() const { return this; }
    const Node* node() const { return this; }
    URL mediaURL() const override { return m_url; }
    const MimeType mimeType() const 
    { 
        Rv::RvSession* session = Rv::RvSession::currentRvSession();
        return session->userGenericEvent("media-library-get-mime-type", m_url); 
    }

    bool isStreaming() const override { return true; }
    bool isRedirecting() const override { return false; }
    const Node* baseMediaNode() const override { return this; }
    HTTPCookieVector httpCookies() const override;
    HTTPHeaderVector httpHeaders() const override;
    URL httpRedirection() const override { return m_url; } ;

  private:
    URL m_url;
};

HTTPCookieVector
EventMediaNode::httpCookies() const
{
    Rv::RvSession* session = Rv::RvSession::currentRvSession();
    std::istringstream sscookies;
    sscookies.str(session->userGenericEvent("media-library-get-http-cookies", m_url));

    HTTPCookieVector cookies;
    for (std::string line; std::getline(sscookies, line); )
    {
        std::vector<std::string> parts;
        const char* str = line.c_str();

        do
        {
            const char* begin = str;

            while (*str != ';' && *str) str++;
            parts.push_back(std::string(begin, str));
        } while (0 != *str++);
        
        if (parts.size() == 4) 
        {
            cookies.push_back(HTTPCookie(parts[0], parts[1], parts[2], parts[3]));    
        }
    }
    return cookies;
}

HTTPHeaderVector
EventMediaNode::httpHeaders() const
{
    Rv::RvSession* session = Rv::RvSession::currentRvSession();
    std::istringstream ssheaders;
    ssheaders.str(session->userGenericEvent("media-library-get-http-headers", m_url));

    HTTPHeaderVector headers;
    for (std::string line; std::getline(ssheaders, line); )
    {
        std::vector<std::string> parts;
        const char* str = line.c_str();

        do
        {
            const char* begin = str;

            while (*str != ';' && *str) str++;
            parts.push_back(std::string(begin, str));
        } while (0 != *str++);
        
        if (parts.size() == 2) 
        {
            headers.push_back(HTTPHeader(parts[0], parts[1]));    
        }
    }
    return headers;
}

class EventRootNode : public EventNode
{
  public:
    EventRootNode(EventMediaLibrary* lib) : EventNode(lib, nullptr) {}
    virtual ~EventRootNode() {}
};


//----------------------------------------------------------------------

std::string
nameFromEventNodeType(EventNodeType t)
{
    switch (t)
    {
      default:
      case EventNoType:    return "EventNoType";
      case EventMediaType: return "EventMediaType";
      case EventRootType:  return "EventRootType";
    }

    return "";
}

EventNode::EventNode(EventMediaLibrary* lib, EventNode* parent)
    : Node(lib),
      m_parent(parent)
{
    if (m_parent) m_parent->addChild(this);
}

EventNode::~EventNode()
{
}

void
EventNode::addChild(EventNode* child)
{
    //
    //  Make me thread safe
    //
    m_children.push_back(child);
}

const EventMediaLibrary* EventNode::lib() const { return dynamic_cast<const EventMediaLibrary*>(m_library); }
EventMediaLibrary* EventNode::lib() { return dynamic_cast<EventMediaLibrary*>(m_library); }


bool EventNode::isValid() const { return true; }
std::string EventNode::typeName() const { return nameFromEventNodeType(m_type); }
std::string EventNode::description() const { return ""; }
std::string EventNode::name() const { return m_name; }
const Node* EventNode::parent() const { return m_parent; }
size_t EventNode::numChildren() const { return m_children.size(); }
const Node* EventNode::child(size_t index) const { return index < m_children.size() ? m_children[index] : nullptr; }
void EventNode::setName(const std::string& n) { m_name = n; }
void EventNode::setParent(const Node* p) { } // not really happening yet
InternalPath EventNode::path() const { return InternalPath(); }
InternalPath EventNode::mediaPath() const { return InternalPath(); }
LibraryURL EventNode::url() const { return LibraryURL(); }
LibraryURL EventNode::persistentURL() const { return LibraryURL(); }
URL EventNode::thumbnailImageURL() const { return URL(); }
MimeType EventNode::mediaMimeType() const { return MimeType(); }
URL EventNode::mediaURL() const { return URL(); }

//----------------------------------------------------------------------

EventMediaLibrary::EventMediaLibrary(const std::string& appName)
    : Library("event-media-library", appName),
      m_root(new EventRootNode(this))
{
}

EventMediaLibrary::~EventMediaLibrary()
{
    delete m_root;
}

const Node* EventMediaLibrary::rootNode() const { return m_root; }
bool EventMediaLibrary::isReadable() const { return true; }
bool EventMediaLibrary::isWriteable() const { return false; }
bool EventMediaLibrary::hasWritableProperties() const { return false; }
bool EventMediaLibrary::hasImport() const { return false; }
bool EventMediaLibrary::hasExport() const { return false; }

size_t EventMediaLibrary::numNodeTypeNames() const { return 2; }

std::string 
EventMediaLibrary::nodeTypeName(size_t i) const
{
    return nameFromEventNodeType(EventNodeType(EventNoType + i + 1));
}

const Node*
EventMediaLibrary::nodeFromInternalPath(const InternalPath& path) const
{
    return nullptr;
}

const Node* 
EventMediaLibrary::nodeFromLibraryURL(const LibraryURL& liburl) const
{
    return nullptr;
}

void EventMediaLibrary::deleteNode(const Node* node) { }
void EventMediaLibrary::reorderNode(const Node* ndoe, size_t newIndex) { }

const NodeAPI*
EventMediaLibrary::nodeAPI(const Node* innode) const
{
    if (const EventMediaNode* node = dynamic_cast<const EventMediaNode*>(innode))
    {
        return node->nodeAPI();
    }

    return nullptr;
}

bool
EventMediaLibrary::isLibraryMediaURL(const URL& inURL) const
{
    Rv::RvSession* session = Rv::RvSession::currentRvSession();
    std::string response = session->userGenericEvent("media-library-is-url", inURL.c_str());

    return response == "true";
}

NodeVector
EventMediaLibrary::associatedNodes(const URL& inURL) const
{
    NodeVector nodes;

    if (m_streamingMap.find(inURL) != m_streamingMap.end())
    {
        //
        //  If already cached return it
        //
        nodes.push_back(m_streamingMap[inURL]);
    }
    else if (isLibraryMediaURL(inURL))
    {
        EventMediaNode* node = new EventMediaNode(const_cast<EventMediaLibrary*>(this),
                                                  inURL,
                                                  m_root);
        m_streamingMap[inURL] = node;
        nodes.push_back(node);
    }
    
    return nodes;
}

} // TwkMediaLibrary
