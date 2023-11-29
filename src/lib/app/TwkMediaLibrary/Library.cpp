//
//  Copyright (c) 2015 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <TwkMediaLibrary/Library.h>
#include <sstream>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FNV1a.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/ThreadName.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iterator>
#include <iomanip>

//
//  NOTE: using QtCore in order to use QUrl. boost doesn't have a URL
//  type -- the other option would be cpp-netlib
//

namespace TwkMediaLibrary {
using namespace std;
using namespace boost;

namespace {
LibraryMap globalLibraryMap;
}

Task::~Task() {}

Node::Node(Library* lib) : m_library(lib) { }
Node::~Node() { }

bool Node::isValid() const { return true; }
string Node::typeName() const { return ""; }
string Node::description() const { return ""; }
std::string Node::name() const { return ""; }
const Node* Node::parent() const { return 0; }
size_t Node::numChildren() const { return 0; }
const Node* Node::child(size_t index) const { return 0; }
void Node::setName(const std::string&) {}
void Node::setParent(const Node*) {}
InternalPath Node::path() const { return InternalPath(); }
InternalPath Node::mediaPath() const { return InternalPath(); }
LibraryURL Node::url() const { return LibraryURL(); }
LibraryURL Node::persistentURL() const { return LibraryURL(); }
URL Node::thumbnailImageURL() const { return URL(); }
MimeType Node::mediaMimeType() const { return MimeType(); }
URL Node::mediaURL() const { return URL(); }
size_t Node::numKeys() const { return 0; }
PropertyKey Node::key(size_t index) const { return PropertyKey(); }
bool Node::hasProperty(const PropertyKey&) const { return false; }
PropertyValue Node::propertyValue(const PropertyKey&) const { return PropertyValue(); }
bool Node::deleteProperty(const PropertyKey&) { return true; }
void Node::setProperty(const PropertyKey&, PropertyType type, const PropertyValue& value) { }
void Node::setPropertyFromString(const PropertyKey&, PropertyType type, const string& value) { }
PropertyType Node::propertyType(const PropertyKey&) const { return StringType; }
size_t Node::numMetaKeys(const PropertyKey&) const { return 0; }
PropertyKey Node::metaKey(const PropertyKey&, size_t index) const { return 0; }
PropertyValue Node::metaValue(const PropertyKey&, const PropertyKey& metaKey) const { return PropertyValue(); }
PropertyType Node::metaType(const PropertyKey&, const PropertyKey& metaKey) const { return StringType; }

size_t
Node::index() const
{
    //
    //  Not ideal implementation, but its prob better not to cache the
    //  index value.
    //

    if (const Node* p = parent())
    {
        for (size_t i = 0, s = p->numChildren(); i < s; i++)
        {
            if (p->child(i) == this) return i;
        }
    }

    return size_t(-1);
}


PropertyKeyVector
Node::keys() const
{
    //
    //  Use the API to provide a default implementation for this
    //

    size_t s = numKeys();
    PropertyKeyVector vec(s);
    for (size_t i = 0; i < s; i++) vec[i] = key(i);
    return vec;
}


PropertyKeyVector
Node::metaKeys(const PropertyKey& key) const
{
    //
    //  Use the API to provide a default implementation for this
    //

    size_t s = numMetaKeys(key);
    PropertyKeyVector vec(s);
    for (size_t i = 0; i < s; i++) vec[i] = metaKey(key, i);
    return vec;
}

bool
Node::hasMeta(const PropertyKey& key, const PropertyKey& meta) const
{
    size_t s = numMetaKeys(key);
    for (size_t i = 0; i < s; i++) if (metaKey(key, i) == meta) return true;
    return false;
}

string
Node::propertyValueAsString(const PropertyKey& key) const
{
    ostringstream str;
    PropertyValue value = propertyValue(key);

    switch (propertyType(key))
    {
      case UnknownType: str << "Unknown"; break;
      case IntType:     str << any_cast<int>(value); break;
      case BoolType:    str << any_cast<bool>(value); break;
      case FloatType:   str << any_cast<float>(value); break;
      default:
      case DateAndTimeType:
      case ReferenceType:
      case URLType:
      case StringType:  str << any_cast<string>(value); break;
      case URLListType:
      case StringListType:
      case ReferenceListType:
          {
              StringVector v = any_cast<StringVector>(value);
              for (size_t i = 0; i < v.size(); i++) str << (i?", ":"") << v[i];
              break;
          }
      case IntListType:
          {
              IntVector v = any_cast<IntVector>(value);
              for (size_t i = 0; i < v.size(); i++) str << (i?", ":"") << v[i];
              break;
          }
    }

    return str.str();
}

string
Node::metaValueAsString(const PropertyKey& key, const PropertyKey& metaKey) const
{
    ostringstream str;
    PropertyValue value = metaValue(key, metaKey);

    switch (metaType(key, metaKey))
    {
      case UnknownType: str << "Unknown"; break;
      case IntType:     str << any_cast<int>(value); break;
      case BoolType:    str << any_cast<bool>(value); break;
      case FloatType:   str << any_cast<float>(value); break;
      default:
      case DateAndTimeType:
      case ReferenceType:
      case URLType:
      case StringType:  str << any_cast<string>(value); break;
      case URLListType:
      case StringListType:
      case ReferenceListType:
          {
              StringVector v = any_cast<StringVector>(value);
              for (size_t i = 0; i < v.size(); i++) str << (i?", ":"") << v[i];
              break;
          }
      case IntListType:
          {
              IntVector v = any_cast<IntVector>(value);
              for (size_t i = 0; i < v.size(); i++) str << (i?", ":"") << v[i];
              break;
          }
    }

    return str.str();
}

bool 
Node::hasAncestor(const Node* node) const
{
    for (const Node* p = parent(); p; p = p->parent())
    {
        if (p == node) return true;
    }

    return false;
}

const Node*
Node::findChildByName(const string& name) const
{
    for (size_t i = 0, s = numChildren(); i < s; i++)
    {
        const Node* node = child(i);
        if (node->name() == name) return node;
    }

    return 0;
}

ostream&
Node::output(ostream& out) const
{
    StringPairVector pairs;

    pairs.push_back( StringPair("Name", name()) );
    pairs.push_back( StringPair("Type", typeName()) );
    pairs.push_back( StringPair("URL", url()) );
    pairs.push_back( StringPair("persistentURL", persistentURL()) );
    pairs.push_back( StringPair("MediaURL", mediaURL()) );
    pairs.push_back( StringPair("ThumbnailImageURL", thumbnailImageURL()) );
    pairs.push_back( StringPair("InteralPath", path()) );
    pairs.push_back( StringPair("InternalMediaPath", mediaPath()) );
    pairs.push_back( StringPair("-", "") );

    for (size_t i = 0, s = numKeys(); i < s; i++)
    {
        PropertyKey k = key(i);
        pairs.push_back( StringPair(k, propertyValueAsString(k)) );
    }

    size_t keySize = 0;

    for (size_t i = 0; i < pairs.size(); i++)
    {
        keySize = std::max(keySize, pairs[i].first.size());
        string::size_type p;

        if ( (p = pairs[i].second.find('\n')) != string::npos )
        {
            pairs[i].second[p] = ' ';
        }
    }

    for (size_t i = 0; i < pairs.size(); i++)
    {
        ostringstream str;
        const string& key   = pairs[i].first;
        const string& value = pairs[i].second;
        str << setfill(key == "-" ? '-' : ' ') << setw(keySize + 1) << key;
        out << str.str() << " = " << value << endl;
    }

    return out;
}

//----------------------------------------------------------------------

NodeAPI::~NodeAPI() {}
CapabilityAPI::~CapabilityAPI() {}
UserAPI::~UserAPI() {}
OrganizationAPI::~OrganizationAPI() {}
WatermarkAPI::~WatermarkAPI() {}
CutAPI::~CutAPI() {}
NoteAPI::~NoteAPI() {}
MediaAPI::~MediaAPI() {}
HTTPCookieVector MediaAPI::httpCookies() const { return HTTPCookieVector(); }
HTTPHeaderVector MediaAPI::httpHeaders() const { return HTTPHeaderVector(); }
URL MediaAPI::httpRedirection() const { return URL(); }

//----------------------------------------------------------------------

Library::Library(const string& typeName, const string& appName)
    : m_typeName(typeName),
      m_appName(appName),
      m_name(typeName),
      m_taskStop(false),
      m_taskThread(threadTrampoline, this)
{
    // globalLibraryMap[typeName] = this;
}

Library::~Library()
{
    {
        ScopedLock lock(m_taskMutex);
        m_taskStop = true;
        m_taskCond.notify_one();
    }

    m_taskThread.join();

    //
    //  willDeleteSignal() should be sent by derived classes. Its sent
    //  here as a stop-gap but if this is used is almost certain going
    //  to be a core dump.
    //
    //  In other words: if you see this comment on the stack of a core
    //  dump where willDeleteSignal() is being sent you now know why.
    //

    willDeleteSignal()();
    globalLibraryMap.erase(name());
}

const LibraryMap& allLibraries() { return globalLibraryMap; }

void
Library::setName(const string& name)
{
    // THIS IS AWFUL AND WE NEED TO CHANGE IT. 

    // because the library initially sets itself to the typeName, it will 
    // continually re-write over "local".  So instead I'm hacking this to 
    // do it when you set the name.  I'm putting it outside the if because 
    // other wise "local" won't get registered and I'm too tired to think of 
    // the right way to do it now. sorry future person.
    LibraryMap::const_iterator i = globalLibraryMap.find(name);
    if (i != globalLibraryMap.end()) globalLibraryMap.erase(name);

    if (m_name != name)
    {
        m_name = name;
        m_nameChangedSignal(m_name);

    }

    globalLibraryMap[m_name] = this;
}

//
//  These all return error or default values
//

bool Library::isReadable() const { return true; }
bool Library::isWriteable() const { return false; }
bool Library::hasWritableProperties() const { return false; }
bool Library::hasImport() const { return false; }
bool Library::hasExport() const { return false; }
URL Library::url() const { return URL(); }
size_t Library::numNodeTypeNames() const { return 0; }
string Library::nodeTypeName(size_t) const { return ""; }
const Node* Library::rootNode() const { return 0; }
const Node* Library::nodeFromLibraryURL(const URL&) const { return 0; }
const Node* Library::nodeFromInternalPath(const InternalPath&) const { return 0; }
void Library::deleteNode(const Node*) { }
bool Library::isolateNode(const Node*) { return false; }
void Library::restoreIsolatedNode(const Node*) { }
void Library::reorderNode(const Node*, size_t newIndex) { }
void Library::moveNode(const Node*, const Node*, size_t newIndex) { }
const Node* Library::newNode(const std::string& nodeTypeName) { return 0; }
const Node* Library::newNodeFromMediaURL(const URL& url) { return 0; }
const Node* Library::newNodeFromCutURL(const URL& url) { return 0; }
URL Library::exportMedia(const Node*) const { return URL(); }
void Library::setNodeName(const Node*, const string&) {}
void Library::setNodeParent(const Node*, const Node*) {}
bool Library::isMediaSupported(const URL& url) const { return false; }
string Library::nodeTypeForMedia(const URL& url) const { return ""; }
bool Library::copyToLibrary(const Node*, const PropertyKey&) { return false; }
bool Library::isAuthenticated() const { return false; }
bool Library::authenticate(const std::string& userName, const std::string& password) const { return false; }
const CapabilityAPI* Library::authenticatedCapabilityAPI() const { return NULL; }
const CapabilityAPI* Library::owner() const { return NULL; }
UserAPIVector Library::userNodeAPIs() const { return UserAPIVector(); }
OrganizationAPIVector Library::organizationNodeAPIs() const { return OrganizationAPIVector(); }
WatermarkAPIVector Library::watermarkNodeAPIs() const { return WatermarkAPIVector(); }
const NodeAPI* Library::nodeAPI(const Node*) const { return NULL; }

URL
libraryURLtoMediaURL(const URL& inURL)
{
    if (const Node* node = nodeOfURL(inURL))
    {
        return node->mediaURL();
    }

    return "nothing";
}

bool
isLibraryURL(const URL& inURL)
{
    return libraryOfURL(inURL) != NULL;
}

bool 
isNodeURL(const URL& inURL)
{
    if (Library* l = libraryOfURL(inURL))
    {
        QUrl url(inURL.c_str());
        return url.fragment() == "";
    }

    return false;
}

bool 
isPropertyURL(const URL& inURL)
{
    if (Library* l = libraryOfURL(inURL))
    {
        QUrl url(inURL.c_str());
        return url.fragment() != "";
    }

    return false;
}

string
Library::nameFromMedia(const URL& inURL)
{
    QUrl url(inURL.c_str());
    QFileInfo info(url.path());
    return info.completeBaseName().toUtf8().constData();
}

string
Library::filenameOfMedia(const URL& inURL)
{
    QUrl url(inURL.c_str());
    QFileInfo info(url.path());
    return info.fileName().toUtf8().constData();
}

MediaFileInfo 
Library::computeMediaFileInfo(const URL& mediaURL)
{
    string mediaFile = mediaURL.substr(7, string::npos);

    TwkUtil::Timer timer;
    timer.start();
    TwkUtil::FileStream fileStream(mediaFile, TwkUtil::FileStream::MemoryMap);
    size_t hashid = TwkUtil::FNV1a64(fileStream.data(), fileStream.size());
    timer.stop();

    ostringstream dataIDstr;
    dataIDstr << "FNV1a/" << hex << hashid;

    MediaFileInfo info;
    info.hashValue         = dataIDstr.str();
    info.hashTimeInSeconds = float(timer.elapsed());
    info.fileSize          = fileStream.size();

    return info;
}

namespace {

void
recursiveFindReferences(const string& refNodeURL, const Node* node, Library::NodePropertyResult& result)
{
    Library::NodePropertyKeyVectorPair propPair;
    PropertyKeyVector keys = node->keys();

    for (size_t i = 0, s = keys.size(); i < s; i++)
    {
        PropertyKey key = keys[i];
        PropertyType t = node->propertyType(key);

        if (t == ReferenceType)
        {
            URL url = any_cast<URL>(node->propertyValue(key));

            if (url == refNodeURL)
            {
                propPair.second.push_back(key);
                propPair.first = node;
            }
        }
        else if (t == ReferenceListType)
        {
            vector<string> v = any_cast< vector<string> >(node->propertyValue(key));
            
            for (size_t q = 0; q < v.size(); q++)
            {
                if (v[q] == refNodeURL)
                {
                    propPair.second.push_back(key);
                    propPair.first = node;
                    break;
                }
            }
        }
    }

    if (propPair.first) result.push_back(propPair);

    for (size_t i = 0, s = node->numChildren(); i < s; i++)
    {
        recursiveFindReferences(refNodeURL, node->child(i), result);
    }
}

}

const Node* 
Library::nodeOfMedia(const URL& url) const
{
    return NULL;
}

bool
Library::isLibraryMediaURL(const URL& inURL) const
{
    //
    //  This is intended to be a fast test. The default case does a full look up.
    //

    return nodeOfMedia(inURL) != NULL;
}

NodeVector 
Library::associatedNodes(const URL& inURL) const
{
    QUrl url(inURL.c_str());
    const Node* node = url.scheme() == "file" ? nodeOfMedia(inURL) : nodeOfURL(inURL);
    NodeVector nodes;

    if (node)
    {
        set<const Node*> nodeSet;

        NodePropertyResult result = referencesToNode(node);
        for (size_t i = 0; i < result.size(); i++) nodeSet.insert(result[i].first);
        std::copy(nodeSet.begin(), nodeSet.end(), back_inserter(nodes));
    }

    return nodes;
}

const Task*
Library::nodeOfMediaASync(const URL&, NodeQueryResultFunction) const
{
    return 0;
}

Library::NodePropertyResult 
Library::referencesToNode(const Node* node) const
{
    //
    //  NOTE: this is a *terrible* way to do this. Each library
    //  implementation would presumably have a much more efficient way of
    //  doing this querry. The default implementation (scan every node in
    //  the library) works only as a stand in.
    //

    NodePropertyResult result;
    recursiveFindReferences(node->url(), rootNode(), result);
    return result;
}

const Task*
Library::referencesToNodeASync(const Node*, PropertyQueryResultFunction) const
{
    return 0;
}

void 
Library::threadTrampoline(Library* library) 
{ 
    library->threadMain(); 
}

void
Library::addTask(Task* item) const
{
    ScopedLock lock(m_taskMutex);
    m_taskQueue.push_back(item);
    m_taskCond.notify_one();
}

void
Library::cancelTask(const Task* item) const
{
    if (item && item->library() == this)
    {
        ScopedLock lock(m_taskMutex);

        TaskDeque::iterator i = find(m_taskQueue.begin(), m_taskQueue.end(), item);

        if (i != m_taskQueue.end())
        {
            delete *i;
            m_taskQueue.erase(i);
        }

        m_taskCond.notify_one();
    }
}

void
Library::cancelTasks(const TaskVector& tasks) const
{
    if (!tasks.empty())
    {
        //
        //  One lock many cancels
        //

        ScopedLock lock(m_taskMutex);

        for (size_t q = 0; q < tasks.size(); q++)
        {
            const Task* item = tasks[q];

            if (item && item->library() == this)
            {
                TaskDeque::iterator i = find(m_taskQueue.begin(), m_taskQueue.end(), item);
        
                if (i != m_taskQueue.end())
                {
                    delete *i;
                    m_taskQueue.erase(i);
                }
            }
        }
        
        m_taskCond.notify_one();
    }
}

void 
Library::threadMain()
{
    //
    //  Worker thread parks in here
    //

    TwkUtil::setThreadName("Library Task Thread");

    ScopedLock lock(m_taskMutex);
    m_taskStop = false;

    while (!m_taskStop) 
    {
        while (!m_taskStop && m_taskQueue.empty()) m_taskCond.wait(lock);

        if (!m_taskStop && !m_taskQueue.empty())
        {
            Task* item = m_taskQueue.front();
            m_taskQueue.pop_front();
            lock.unlock();

            try
            {
                item->execute();
            }
            catch (const std::exception& exc)
            {
                cout << "ERROR: Library worker thread caught: " << exc.what() << endl;
            }
            catch (...)
            {
                cout << "ERROR: Library worker thread uncaught exception" << endl;
            }
            
            delete item;
            lock.lock();
        }
    }
}

Library*
libraryOfURL(const URL& inURL)
{
    QUrl url(inURL.c_str());

    if (url.scheme() == "sglib")
    {
        string libname = url.host().toUtf8().constData();
        LibraryMap::const_iterator i = globalLibraryMap.find(libname);
        if (i != globalLibraryMap.end()) return i->second;
    }

    return 0;
}

const Node*
nodeOfURL(const URL& inURL)
{
    if (Library* l = libraryOfURL(inURL))
    {
        if (const Node* node = l->nodeFromLibraryURL(inURL))
        {
            return node;
        }
        else
        {
            QUrl url(inURL.c_str());
            if (url.path() == "") return l->rootNode();
        }
    }

    return 0;
}

NodeVector 
libraryNodesAssociatedWithURL(const URL& inURL)
{
    NodeVector nodes;

    for (LibraryMap::const_iterator i = globalLibraryMap.begin();
         i != globalLibraryMap.end();
         ++i)
    {
        NodeVector n = i->second->associatedNodes(inURL);
        std::copy(n.begin(), n.end(), back_inserter(nodes));
    }

    return nodes;
}

URLVector
libraryURLsAssociatedWithURL(const URL& inURL)
{
    URLVector urls;

    for (LibraryMap::const_iterator i = globalLibraryMap.begin();
         i != globalLibraryMap.end();
         ++i)
    {
        NodeVector n = i->second->associatedNodes(inURL);

        for (size_t q = 0; q < n.size(); q++) urls.push_back(n[q]->persistentURL());
    }

    return urls;
}

URLVector
libraryURLsOfMedia(const URL& mediaURL)
{
    URLVector urls;

    for (LibraryMap::const_iterator i = globalLibraryMap.begin();
         i != globalLibraryMap.end();
         ++i)
    {
        if (const Node* node = i->second->nodeOfMedia(mediaURL))
        {
            urls.push_back(node->persistentURL());
        }
    }

    return urls;
}

bool
isLibraryMediaURL(const URL& mediaURL)
{
    for (LibraryMap::const_iterator i = globalLibraryMap.begin();
         i != globalLibraryMap.end();
         ++i)
    {
        if (i->second->isLibraryMediaURL(mediaURL)) return true;
    }

    return false;
}

} // TwkMediaLibrary
