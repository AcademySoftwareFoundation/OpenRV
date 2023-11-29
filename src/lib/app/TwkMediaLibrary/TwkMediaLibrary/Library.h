//
//  Copyright (c) 2015 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __TwkMediaLibrary__Library__h__
#define __TwkMediaLibrary__Library__h__
#include <iostream>
#include <set>
#include <deque>
#include <map>
#include <boost/signals2.hpp>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

namespace TwkMediaLibrary {

/// Module Types

//
//  URL             :   Any scheme
//  LibraryURL      :   Library scheme
//  InternalPath    :   UNIX style path relative to library root --
//                      may or may not correspond to equivalent
//                      LibraryURL
//


struct HTTPCookie
{
    HTTPCookie(const std::string& d,
               const std::string& p,
               const std::string& n,
               const std::string& v)
        : domain(d),
          path(p),
          name(n),
          value(v) {}
    
    std::string domain;
    std::string path;
    std::string name;
    std::string value;
};

struct HTTPHeader
{
    HTTPHeader(std::string n,
               std::string v)
        : name(std::move(n)),
          value(std::move(v)) {}
    
    std::string name;
    std::string value;
};

//
//  All std::strings are assumed to be UTF-8 encoded *except* for URLs.
//  These should be the subset of UTF-8 strings producing valid URLs.
//

typedef std::string                         URL;
typedef std::string                         LibraryURL;
typedef std::string                         InternalPath;
typedef std::string                         FileSystemPath;
typedef std::string                         MimeType;
typedef boost::any                          PropertyValue;
typedef std::vector<boost::any>             PropertyValueVector;
typedef std::string                         PropertyKey;
typedef std::vector<PropertyKey>            PropertyKeyVector;
typedef std::vector<URL>                    URLVector;
typedef std::vector<std::string>            StringVector;
typedef std::vector<int>                    IntVector;
typedef std::pair<std::string, std::string> StringPair;
typedef std::vector<StringPair>             StringPairVector;
typedef std::vector<HTTPCookie>             HTTPCookieVector;
typedef std::vector<HTTPHeader>             HTTPHeaderVector;

enum PropertyType
{
    UnknownType,

    FloatType,
    IntType,
    StringType,
    BoolType,
    ReferenceType, 
    URLType,
    DateType,
    DateAndTimeType,

    IntListType,
    StringListType,
    ReferenceListType, 
    URLListType,

    FirstListType = IntListType

    // FIELD TYPES (shotgun)
    // ---------------------
    // DateType,
    // DateAndTime,
    // Duration,
    // Entity,
    // Multi-Entity,
    // File/Link
    // Footage
    // List (enum)
    // Number (???)
    // Percent
    // Query
    // Status List
    // Text
    // Timecode
    // URL Template

    // ATTRIBUTES (mac media libs)
    // ---------------------------
};

enum PropertyFlags
{
    ReadFlag        = 1 << 0,
    WriteFlag       = 1 << 1,
    HiddenFlag      = 1 << 2,
};


struct MediaFileInfo
{
    std::string hashValue;
    float       hashTimeInSeconds;
    size_t      fileSize;
};


enum UserCapabilityFlags
{
    AdministratorCapabilityFlag = 1 << 0,
    NoWatermarkCapabilityFlag   = 1 << 1, // no watermarks while viewing media
    ExportCapabilityFlag        = 1 << 2, // can export to movie/audio files
    RawExportCapabilityFlag     = 1 << 3, // can export raw media files
    EditCapabilityFlag          = 1 << 4, // can edit library

    DefaultCapabiliesFlag       =   AdministratorCapabilityFlag
                                  | NoWatermarkCapabilityFlag
                                  | ExportCapabilityFlag 
                                  | RawExportCapabilityFlag 
                                  | EditCapabilityFlag
};

enum TaskType
{
    QueryTaskType,
    EditingTaskType
};

class Library;

///  class Node is a handle to an object in the library

///
///  class Node
///

class Node
{
  public:
    //
    //  Status
    //

    virtual bool isValid() const;
    virtual std::string name() const;
    virtual std::string typeName() const;
    virtual std::string description() const;

    //
    //  Hierarchy
    //

    virtual const Node* parent() const;
    virtual size_t numChildren() const;
    virtual const Node* child(size_t index) const;
    virtual const Library* library() const { return m_library; }
    virtual size_t index() const;
    virtual const Node* findChildByName(const std::string&) const;
    virtual bool hasAncestor(const Node*) const;

    //
    //  InternalPaths to the node and related media. These are paths
    //  relative to the library in the hierarchy space.
    //
    //  Internal Paths look like:
    //
    //      /root/path/to/the/node
    //
    //  they are abstract and don't necessarily correspond to real
    //  file system paths. They may or may not be valid Library URLs
    //  if prefixed by the sglib:// scheme. That's up to the library
    //  implementation. 
    //
    //  PropertyValues that cross reference nodes may use InternalPath
    //
    //  path()          : the path to this node
    //  mediaPath()     : path to the "current" media node (could be path())
    //

    virtual InternalPath path() const;
    virtual InternalPath mediaPath() const;

    //
    //  Globally accessible locations. Only url() will return a library
    //  URL. The other return a "browsable" URL. An actual object should
    //  exist at the URL.
    //
    //  Some examples of url() might be:
    //
    //  ShotGrid Server : sglib://shotgun/server.shotgridsoftware.com/path/to/a/node
    //  Local           : sglib://local/path/to/a/node
    //
    //  Some examples of mediaURL() or thumbnailImageURL() might be:
    //
    //  ShotGrid Server : http://server.shotgridsoftware.com/path/to/a/node?media
    //  Local           : file:///Users/name/Movies/mylibrary.sglib/node21/hires.mov
    //  Local           : file:///Users/name/Movies/mylibrary.sglib/node32131/thumbnail.jpg
    //

    virtual LibraryURL url() const;
    virtual LibraryURL persistentURL() const;
    virtual URL mediaURL() const;
    virtual URL thumbnailImageURL() const;

    //
    //  MimeType is a two-part identifier for file formats and format
    //  contents (from Wikipedia).
    //

    virtual MimeType mediaMimeType() const;

    //
    //  Properties
    //  Property Paths: sglib://path/to/node#propertykey
    //
    //  The schema used by a node to hold its state should not be assumed
    //  by code using the MediaLibrary API. To get semantic information
    //  from a node without needing to know about the schema use one of the
    //  API objects below. E.g. the UserAPI will give you the name of a
    //  user without needing to know which property or properties store
    //  that information. 
    //

    virtual size_t numKeys() const;
    virtual PropertyKey key(size_t index) const;
    virtual PropertyKeyVector keys() const;
    virtual bool hasProperty(const PropertyKey&) const;
    virtual PropertyValue propertyValue(const PropertyKey&) const;
    virtual PropertyType propertyType(const PropertyKey&) const;
    std::string propertyValueAsString(const PropertyKey&) const;

    //
    //  Metaproperties
    //
    //  These are properties *of* properties. They are assumed to be
    //  constant and vary with node type. An example use might be to
    //  provide a specific default value or a list of enum values for the
    //  UI, etc, etc.
    //

    virtual size_t numMetaKeys(const PropertyKey&) const;
    virtual PropertyKey metaKey(const PropertyKey&, size_t index) const;
    virtual PropertyKeyVector metaKeys(const PropertyKey&) const;
    virtual bool hasMeta(const PropertyKey&, const PropertyKey&) const;
    virtual PropertyValue metaValue(const PropertyKey&, const PropertyKey& metaKey) const;
    virtual PropertyType metaType(const PropertyKey&, const PropertyKey& metaKey) const;
    std::string metaValueAsString(const PropertyKey&, const PropertyKey&) const;

    //
    //  Utilities
    //

    std::ostream& output(std::ostream&) const;

  protected:
    Node(Library*);
    virtual ~Node();

    //
    //  Mutating functions. These should only be called through the edit
    //  object
    //

    virtual bool deleteProperty(const PropertyKey&);
    virtual void setProperty(const PropertyKey&, PropertyType type, const PropertyValue& value);
    virtual void setPropertyFromString(const PropertyKey&, PropertyType type, const std::string& value);
    virtual void setName(const std::string&);
    virtual void setParent(const Node*);

    friend class Library;
    friend class NodeEdit;

  protected:
    Library* m_library;
};

typedef std::vector<const Node*> NodeVector;


/// NodeAPI adds specific API to Node via a separate object

class NodeAPI
{
  public:
    virtual const Node* node() const = 0;

  protected:
    NodeAPI() {}
    virtual ~NodeAPI();
};

/// CapabilityAPI makes

///
/// class CapabilityAPI
///
/// Nodes which can be used to authenticate are CapabilityNodes. Right now
/// that's just UserNode and OrganizationNode.
///

class CapabilityAPI;
class UserAPI;
class OrganizationAPI;
class WatermarkAPI;

typedef std::vector<const UserAPI*>         UserAPIVector;
typedef std::vector<const OrganizationAPI*> OrganizationAPIVector;
typedef std::vector<const WatermarkAPI*>    WatermarkAPIVector;

class CapabilityAPI : public NodeAPI
{
  public:
    virtual std::string loginName() const = 0;
    virtual bool passwordMatches(const std::string& plainTextUTF8) const = 0;
    virtual UserCapabilityFlags capabilities() const = 0;

  protected:
    CapabilityAPI() : NodeAPI() {}
    virtual ~CapabilityAPI();
};

class UserAPI : public CapabilityAPI
{
  public:
    virtual const OrganizationAPI* organization() const = 0;

  protected:
    UserAPI() : CapabilityAPI() {}
    virtual ~UserAPI();
};

class OrganizationAPI : public CapabilityAPI
{
  public:
    virtual UserAPIVector users() const = 0;
    
  protected:
    OrganizationAPI() : CapabilityAPI() {}
    virtual ~OrganizationAPI();
};

class WatermarkAPI : public NodeAPI
{
  public:
    virtual std::string text() const = 0;

  protected:
    WatermarkAPI() : NodeAPI() {}
    virtual ~WatermarkAPI();
};

class CutAPI : public NodeAPI
{
  public:
    virtual int cutStart() const = 0;
    virtual int cutEnd() const = 0;
    virtual float cutFPS() const = 0;
    virtual size_t numClips() const = 0;
    virtual std::string clipMedia(size_t) const = 0;
    virtual const Node* clipNode(size_t) const = 0;
    virtual float clipFPS(size_t) const = 0;
    virtual int clipIn(size_t) const = 0;
    virtual int clipOut(size_t) const = 0;
    virtual int cutIn(size_t) const = 0;
    virtual int cutOut(size_t) const = 0;

  protected:
    CutAPI() : NodeAPI() {}
    virtual ~CutAPI();
};

class NoteAPI : public NodeAPI
{
  public:
    virtual std::string content() const = 0;
    virtual const Node* toUser() const = 0;
    virtual NodeVector ccUsers() const = 0;
    virtual URLVector attachments() const = 0;
    virtual NodeVector objectsOfNote() const = 0;

  protected:
    NoteAPI() : NodeAPI() {}
    virtual ~NoteAPI();
};

class MediaAPI : public NodeAPI
{
  public:
    virtual bool isStreaming() const = 0;
    virtual bool isRedirecting() const = 0;
    virtual const Node* baseMediaNode() const = 0; // iff isProxy() == true
    virtual HTTPCookieVector httpCookies() const;
    virtual HTTPHeaderVector httpHeaders() const;
    virtual URL httpRedirection() const;

  protected:
    MediaAPI() : NodeAPI() {}
    virtual ~MediaAPI();
};

/// class NodeEdit is used on the stack to make mutating changes to a Node

///
/// class NodeEdit
///
/// Usage:
///
///     {
///         const Node* node = ...;
///         NodeEdit edit(node);
///         edit.setName("foo");
///     }
///

class NodeEdit
{
  public:
    NodeEdit(const Node* n)
        : m_node(const_cast<Node*>(n)),
          m_library(n->m_library) {}
    ~NodeEdit() {}

    bool deleteProperty(const PropertyKey& k) { bool b = n()->deleteProperty(k); return b; }
    void setProperty(const PropertyKey& k, PropertyType t, const PropertyValue& v) { n()->setProperty(k,t,v); }
    void setPropertyFromString(const PropertyKey& k, PropertyType t, const std::string& v) { n()->setPropertyFromString(k,t,v); }
    void setName(const std::string& name) { n()->setName(name); }
    void setParent(const Node* p) { n()->setParent(p); }

  protected:
    Node* n() { return m_node; }

  private:
    Node*    m_node;
    Library* m_library;
};

/// Task - an asynchronous parallel task

///
///  This class does *not* manage its own thread. The execute function
///  should be blocking. Task to thread assignment is handled elsewhere. 
///

class Task
{
  public:
    Task(const std::string& name, Library* l, TaskType t) 
        : m_name(name), m_library(l), m_type(t) {}
    virtual ~Task();

    Library* library() const { return m_library; }
    TaskType type() const { return m_type; }
    const std::string& name() const { return m_name; }
    virtual void execute() = 0;

  private:
    Library*    m_library;
    TaskType    m_type;
    std::string m_name;
};

typedef std::vector<const Task*> TaskVector;

///  class Library is the base API for a media library

///
///  class Library
///

class Library
{
  public:
    //
    //  Types
    //

    typedef void (VoidFunc)(void);
    typedef void (NodeFunc)(const Node*);
    typedef void (NodePairFunc)(const Node*, const Node*);
    typedef void (NodeStringFunc)(const Node*, const std::string&);
    typedef void (NodeSizetFunc)(const Node*, size_t);
    typedef void (StringFunc)(const std::string&);
    typedef void (NodeMoveFunc)(const Node*, const Node*, size_t, const Node*, size_t);

    typedef boost::signals2::signal<VoidFunc>         VoidSignal;
    typedef boost::signals2::signal<NodeFunc>         NodeSignal;
    typedef boost::signals2::signal<NodePairFunc>     NodePairSignal;
    typedef boost::signals2::signal<NodeStringFunc>   NodePropertySignal;
    typedef boost::signals2::signal<NodeSizetFunc>    NodeIndexSignal;
    typedef boost::signals2::signal<NodeMoveFunc>     NodeMoveSignal;
    typedef boost::signals2::signal<StringFunc>       StringSignal;
    typedef std::pair<const Node*, PropertyKeyVector> NodePropertyKeyVectorPair;
    typedef std::vector<NodePropertyKeyVectorPair>    NodePropertyResult;
    typedef boost::mutex                              Mutex;
    typedef boost::mutex::scoped_lock                 ScopedLock;
    typedef boost::thread                             Thread;
    typedef boost::condition_variable                 Condition;
    typedef std::deque<Task*>                         TaskDeque;

    typedef void (NodePropertyResultFunc)(const Task*, const NodePropertyResult&);
    typedef void (NodeTaskFunc)(const Task*, const Node*);

    typedef boost::function<NodePropertyResultFunc> PropertyQueryResultFunction;
    typedef boost::function<NodeTaskFunc>           NodeQueryResultFunction;

    //
    //  Constructors
    //

    explicit Library(const std::string& typeName, const std::string& appName);
    virtual ~Library();

    //
    //  Capabilities
    //

    virtual bool isReadable() const;
    virtual bool isWriteable() const;
    virtual bool hasWritableProperties() const;
    virtual bool hasImport() const;
    virtual bool hasExport() const;

    //
    //  Signals
    //
    //  NOTE: willDeleteSignal() should be sent by the most derived class'
    //  destructor before any state changes occur (even before that). The
    //  base class will also send it, but if its used that's almost a
    //  guaranteed core dump
    //
    //  nodePropertiesWillBeAddedSignal() and nodePropertiesAddedSignal()
    //  should be sent when unique node properties are created on a single
    //  node. In the case that the schema changes use
    //  nodeSchemeChangedSignal() and nodeSchemeWillChangeSignal() in which
    //  case the node argument will be the scheme itself *not* a node in
    //  the library.
    //

    VoidSignal& willDeleteSignal() { return m_willDeleteSignal; }
    NodeSignal& nodeTreeWillDeleteSignal() { return m_nodeTreeWillDeleteSignal; }
    NodeSignal& nodeTreeDidDeleteSignal() { return m_nodeTreeDidDeleteSignal; }
    NodeSignal& nodeWillDeleteSignal() { return m_nodeWillDeleteSignal; }
    NodeSignal& nodeDidDeleteSignal() { return m_nodeDidDeleteSignal; }
    NodeIndexSignal& willCreateNodeSignal() { return m_willCreateNodeSignal; }
    NodeSignal& newNodeSignal() { return m_newNodeSignal; }
    NodePropertySignal& nodePropertyChangedSignal() { return m_nodePropertyChangedSignal; }
    NodeSignal& nodePropertiesWillBeAddedSignal() { return m_nodePropertiesWillBeAddedSignal; }
    NodeSignal& nodePropertiesAddedSignal() { return m_nodePropertiesAddedSignal; }
    StringSignal& nameChangedSignal() { return m_nameChangedSignal; }
    NodeSignal& nodeNameChangedSignal() { return m_nodeNameChangedSignal; }

    NodePairSignal& willChangeNodeParentSignal() { return m_willChangeNodeParentSignal; }
    NodePairSignal& nodeParentChangedSignal() { return m_nodeParentChangedSignal; }

    NodeIndexSignal& willReorderNodeSignal() { return m_willReorderNodeSignal; }
    NodeSignal& nodeReorderedSignal() { return m_nodeReorderedSignal; }

    NodeSignal& nodeSchemeWillChangeSignal() { return m_nodeShemeWillChangeSignal; }
    NodeSignal& nodeSchemeChangedSignal() { return m_nodeShemeChangedSignal; }

    NodeMoveSignal& willMoveNodeSignal() { return m_willMoveSignal; }
    NodeSignal& nodeMovedSignal() { return m_movedSignal; }

    const std::string& typeName() const { return m_typeName; }
    const std::string& appName() const { return m_appName; }
    const std::string& name() const { return m_name; }

    virtual URL url() const;

    void setName(const std::string&);

    //
    //  Enumerate all node supported node types
    //

    virtual size_t numNodeTypeNames() const;
    virtual std::string nodeTypeName(size_t) const;

    //
    //  Authentication
    //
    //  The user should match either the default user or one in the library
    //  itself. If isAuthenticated() is true than a user has been
    //  authenticated successfully. The authenticatedCapabilityNode() is
    //  the CapabilityNode of the authenticated user/organization.
    //
    //  owner() will return the CapabilityNode of the owner of this library
    //  if there is one.
    //

    virtual bool isAuthenticated() const;
    virtual bool authenticate(const std::string& authenticationName, const std::string& password) const;
    virtual const CapabilityAPI* authenticatedCapabilityAPI() const;
    virtual const CapabilityAPI* owner() const;

    //
    //  Manage Nodes
    //
    //  NOTE: deleteNode() should delete all its ancestors analogous
    //  to shell command: rm -rf. Child ordering should remain stable
    //  if a child is deleted (but obviously some children may have
    //  new child index numbers).
    //

    virtual const Node* newNode(const std::string& nodeTypeName);
    virtual void deleteNode(const Node*);
    virtual void setNodeName(const Node*, const std::string&);
    virtual void setNodeParent(const Node*, const Node*);
    virtual void reorderNode(const Node*, size_t newIndex);
    
    virtual void moveNode(const Node* node, const Node* newParent, size_t newIndex);

    //
    //  Isolating a node does not delete the node but it does make it
    //  inaccessible to the outside world. After calling isolateNode() and
    //  if the function returns true, the pointer to the node is no longer
    //  valid, but has not been deleted; the only function which may be
    //  called with the pointer is restoreIsolatedNode(). If
    //  restoreIsolatedNode() is called the pointer becomes valid again.
    //  When the library is closed, nodes that have been isolated but not
    //  restored will be destroyed permanently. 
    //
    //  This mechanism exists to efficiently implement undo/redo. The
    //  default implementation does nothing -- isolateNode returns false
    //  indicating that it failed (and no changes have occured, the node is
    //  still valid).
    //

    virtual bool isolateNode(const Node*);
    virtual void restoreIsolatedNode(const Node*);

    //
    //  Querries
    //
    //  referencesToNode() blocks and returns a vector of property
    //  URLs. The default implemenation will almost certainly be too slow
    //  for release code (it recursively scans library node properties each
    //  time its called). Each implementation should override it.
    //  Similarily, referencesToNodeASync() is also slow and will consume
    //  too many resources. In the ASync case an internal thread will be
    //  calling the PropertyQueryFunc.
    //
    //  nodeOfMedia() will locate a Media node corresponding to the given
    //  file URL. The default implementation is dog slow and a resource
    //  hog. 
    //

    virtual const Node* rootNode() const;
    virtual const Node* nodeFromInternalPath(const InternalPath&) const;
    virtual const Node* nodeFromLibraryURL(const LibraryURL&) const;

    virtual NodePropertyResult referencesToNode(const Node*) const; 
    virtual const Node* nodeOfMedia(const URL& url) const;
    virtual NodeVector associatedNodes(const URL& url) const;

    virtual const Task* referencesToNodeASync(const Node*, PropertyQueryResultFunction) const; 
    virtual const Task* nodeOfMediaASync(const URL&, NodeQueryResultFunction) const; 

    virtual bool isLibraryMediaURL(const URL&) const;

    //
    //  Import (Ingest) Media
    //
    //  This will almost certainly launch an external process to ingest the
    //  media. 
    //
    //  isMediaSupported()      returns false if its not supported. 
    //  nodeTypeForMedia()      return the name of the node type that
    //                          would be created by newNodeFromMediaURL()
    //  newNodeFromMediaURL()   creates a node of the proper type (which is
    //                          up to the underlying implementation)
    //  newNodeFromCutURL()     creates structures associated with a "cut
    //                          file" which could be an rv/sgr session,
    //                          exchange EDL format, etc.
    //

    virtual bool isMediaSupported(const URL& url) const;
    virtual std::string nodeTypeForMedia(const URL& url) const;
    virtual const Node* newNodeFromMediaURL(const URL& url);
    virtual const Node* newNodeFromCutURL(const URL& url);

    //
    //  copyToLibrary()         copy an extrenally referenced file into 
    //                          the library itself. The property key should
    //                          point to a URLType property.
    //

    virtual bool copyToLibrary(const Node*, const PropertyKey&);

    //
    //  NodeAPI access
    //

    virtual const NodeAPI*          nodeAPI(const Node*) const;
    virtual UserAPIVector           userNodeAPIs() const;
    virtual OrganizationAPIVector   organizationNodeAPIs() const;
    virtual WatermarkAPIVector      watermarkNodeAPIs() const;

    //
    //  In order to do a reverse look-up the media needs identifying
    //  information. This function, given a file, will compute that
    //  information. It does not do anything with it other than return it.
    //

    static MediaFileInfo computeMediaFileInfo(const URL& mediaURL);

    //
    //  Export Media
    //

    virtual URL exportMedia(const Node*) const;

    //
    //  Utility functions
    //

    static std::string nameFromMedia(const URL&);
    static std::string filenameOfMedia(const URL&);

    //
    //  ASync/Parallel Task API
    //

    static void threadTrampoline(Library*);
    void threadMain();

    void addTask(Task*) const;
    void cancelTask(const Task*) const;
    void cancelTasks(const TaskVector&) const;

  protected:
    Library() {}

  private:
    const std::string  m_typeName;
    const std::string  m_appName;
    std::string        m_name;
    VoidSignal         m_willDeleteSignal;
    NodeIndexSignal    m_willCreateNodeSignal;
    NodeSignal         m_newNodeSignal;
    NodeSignal         m_nodeTreeWillDeleteSignal;
    NodeSignal         m_nodeTreeDidDeleteSignal;
    NodeSignal         m_nodeWillDeleteSignal;
    NodeSignal         m_nodeDidDeleteSignal;
    NodePropertySignal m_nodePropertyChangedSignal;
    NodeSignal         m_nodePropertiesWillBeAddedSignal;
    NodeSignal         m_nodePropertiesAddedSignal;
    StringSignal       m_nameChangedSignal;
    NodeSignal         m_nodeNameChangedSignal;

    NodePairSignal     m_nodeParentChangedSignal;
    NodePairSignal     m_willChangeNodeParentSignal;

    NodeIndexSignal    m_willReorderNodeSignal;
    NodeSignal         m_nodeReorderedSignal;

    NodeMoveSignal     m_willMoveSignal;;
    NodeSignal         m_movedSignal;

    NodeSignal         m_nodeShemeWillChangeSignal;
    NodeSignal         m_nodeShemeChangedSignal;
    Thread             m_taskThread;
    mutable Condition  m_taskCond;
    mutable Mutex      m_taskMutex;
    mutable bool       m_taskStop;
    mutable TaskDeque  m_taskQueue;
};

//
//  Global
//

typedef std::map<std::string,Library*> LibraryMap;

bool isLibraryURL(const URL&);
bool isNodeURL(const URL&);
bool isPropertyURL(const URL&);
Library* libraryOfURL(const URL&);
const Node* nodeOfURL(const URL&);
const LibraryMap& allLibraries();
URL libraryURLtoMediaURL(const URL&);
bool isLibraryMediaURL(const URL&);
URLVector libraryURLsOfMedia(const URL&);
NodeVector libraryNodesAssociatedWithURL(const URL&);
URLVector libraryURLsAssociatedWithURL(const URL&);
inline bool isPropertyListType(PropertyType t) { return t >= FirstListType; }

template <class T>
inline const T* api_cast(const Node* node)
{
    if (const NodeAPI* api = node->library()->nodeAPI(node))
    {
        return dynamic_cast<const T*>(api);
    }
    else
    {
        return NULL;
    }
}


} // TwkMediaLibrary

#endif // __TwkMediaLibrary__Library__h__
