//
//  Copyright (c) 2008 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __IPGraph__FileSourceIPNode__h__
#define __IPGraph__FileSourceIPNode__h__
#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/IPGraph.h>
#include <TwkApp/Bundle.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkAudio/Resampler.h>
#include <TwkMovie/Movie.h>

#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <pthread.h>

#include <boost/thread.hpp>

#include <QReadWriteLock>

namespace TwkContainer {
class PropertyContainer;
}

namespace IPCore {

///
///  A FileSourceIPNode can be one or more "Movie" (frame sequence,
///  filter, movie file) objects. One or more of these are evaluated
///  each frame depending on the context (e.g., stereo, stack, etc)
///

class FileSourceIPNode : public SourceIPNode
{
    class Media;
    class SharedMedia;

  public:
    //
    //  Types
    //

    typedef TwkAudio::Time                       Time;
    typedef TwkAudio::SampleTime                 SampleTime;
    typedef TwkMovie::Movie                      Movie;
    typedef std::vector<Movie*>                  MovieVector;
    typedef TwkContainer::FloatProperty          FloatProperty;
    typedef TwkContainer::IntProperty            IntProperty;
    typedef TwkContainer::StringProperty         StringProperty;
    typedef TwkAudio::MultiResampler             Resampler;
    typedef std::vector<float>                   SampleVector;
    typedef std::set<std::string>                StringSet;
    typedef std::recursive_mutex                 Mutex;
    typedef std::lock_guard<Mutex>               LockGuard;
    typedef boost::condition_variable            Condition;
    typedef boost::shared_ptr<Movie>             SharedMoviePointer;
    typedef boost::shared_ptr<PropertyContainer> SharedPropertyContainer;
    typedef std::vector<SharedMoviePointer>      SharedMoviePointerVector;
    typedef boost::shared_ptr<SharedMedia>       SharedMediaPointer;
    typedef std::vector<SharedMediaPointer>      SharedMediaPointerVector;
    typedef boost::shared_ptr<Media>             MediaPointer;
    typedef std::vector<MediaPointer>            MediaPointerVector;
    typedef std::pair<std::string,std::string>   StringPair;
    typedef std::deque<StringPair>               Parameters;

    //
    //  Default values. The defaultFPS is used when no FPS is reported
    //  by a movie object.
    //

    static float defaultFPS;
    static float defaultOverrideFPS;

    //
    //  Constructors
    //

    FileSourceIPNode(const std::string& name,
                     const NodeDefinition* def,
                     IPGraph*,
                     GroupIPNode* group,
                     const std::string mediaRepName="");

    virtual ~FileSourceIPNode();

    //
    //  IPNode API
    //

    virtual IPImage* evaluate(const Context&);
    virtual void testEvaluate(const Context&, TestEvaluationResult&);
    virtual IPImageID* evaluateIdentifier(const Context&);
    virtual void flushAllCaches(const FlushContext&);
    virtual ImageRangeInfo imageRangeInfo() const;
    virtual ImageStructureInfo imageStructureInfo(const Context&) const;
    virtual void prepareForWrite();
    virtual void writeCompleted();
    virtual void readCompleted(const std::string&, unsigned int);
    virtual void propertyChanged(const Property*);
    virtual void propertyDeleted(const std::string&);
    virtual void newPropertyCreated(const Property*);
    virtual void propertyWillChange(const Property*);

    virtual size_t audioFillBuffer(const AudioContext&);

    //
    //  Media API from SourceIPNode
    //

    virtual size_t numMedia() const;
    virtual size_t mediaIndex(const std::string& name) const;
    virtual const MovieInfo& mediaMovieInfo(size_t index) const;
    virtual const std::string& mediaName(size_t index) const;

    //
    //  This returns the "mangled" file name that rv accepts. For
    //  example: left.mov|right.mov means two sources in this node:
    //  foo.mov and bar.mov
    //

    std::string filename() const;

    //
    //  Clear all media
    //

    void clearMedia();

    //
    //  Get the nth movie object
    //

    Movie* movieByMediaIndex(size_t index=0);
    const Movie* movieByMediaIndex(size_t index=0) const;

    //
    //  Get Movie by filename
    //

    Movie* movieByMediaName(const std::string&);
    const Movie* movieByMediaName(const std::string&) const;

    //
    //  FPS of the source
    //

    float fps() const;

    bool isBackwards() const;

    void setReadAllChannels(bool v);

    void invalidateFileSystemInfo();

    //
    //  Media I/O
    //

    void openMovieTask(const std::string& filename, const SharedMediaPointer & proxySharedMedia);
    Movie* openMovie(const std::string& filename);

    bool findCachedMediaMetaData(const std::string& filename, PropertyContainer* pc);

    static std::string cacheHash(const std::string& filename, const std::string& prefix);

    // Lookup filename in the media library, save associated HTTP header and cookies if any, 
    // and return the actual filename to be used for opening the movie in case of redirection.
    std::string lookupFilenameInMediaLibrary(const std::string& filename);

    static void setSourceNameInID(bool b) { m_sourceNameInID = b; }
    static bool sourceNameInID() { return m_sourceNameInID; };

    //
    //  User input parameters
    //

    void storeInputParameters(Parameters p) { m_inparams = p; }
    const Parameters inputParameters() const { return m_inparams; }

    //
    //  Progressive source loading
    //

    bool hasProgressiveSourceLoading() const { return m_progressiveSourceLoading; }
    void setProgressiveSourceLoading(bool b) { m_progressiveSourceLoading = b; }

  protected:
    virtual void audioConfigure(const AudioConfiguration&);
    size_t audioFillBufferInternal(const AudioContext&);
    void addMedia(const SharedMediaPointer& sharedMedia, const SharedMediaPointer & proxySharedMedia = SharedMediaPointer());
    void changeMedia(const SharedMediaPointer& sharedMedia, const SharedMediaPointer & proxySharedMedia = SharedMediaPointer());

    MediaPointer getMediaFromContext(ImageComponent& selection, const Context& context);
    MediaPointer mediaForComponent(ImageComponent&, const Context& context);
    MediaPointer defaultMedia(int);
    Movie* movieForThread(const Media*, const Context&);
    void setupRequest(const Movie*, const ImageComponent&, const Context&, Movie::ReadRequest& request);

    void reloadMediaFromFiles();

    SharedMedia* newSharedMedia(Movie*, bool hasValidRange);

    void updateHasAudioStatus();
    Time offsetStartTime(const AudioContext context);

    void configureAlphaAttrs( FrameBuffer* fb, IPImage* img );
    Movie* openProxyMovie(const std::string& errorString, double minBeforeTime, const std::string filename, double defaultFPS);

    void cancelJobs();

    void setLoadingID( IPGraph::WorkItemID newLoadingID ); 
    void resetLoadingID() { setLoadingID(0); } 

    void addDispatchJob(IPGraph::WorkItemID ID);
    void removeDispatchJob(IPGraph::WorkItemID ID);
    void cancelAllDispatchJobs();
    void undispatchAllDispatchJobs();

  private:
    StringProperty*     m_mediaMovies;
    StringProperty*     m_mediaViewNames;
    MediaPointerVector  m_mediaVector;
    FloatProperty*      m_offset;
    IntProperty*        m_rangeOffset;
    IntProperty*        m_rangeStart;
    IntProperty*        m_noMovieAudio;
    IntProperty*        m_cutIn;
    IntProperty*        m_cutOut;
    FloatProperty*      m_fps;
    FloatProperty*      m_volume;
    FloatProperty*      m_balance;
    FloatProperty*      m_crossover;
    IntProperty*        m_readAllChannels;
    StringVector        m_allViews;
    StringSet           m_viewNameSet;
    Mutex               m_audioMutex;
    TwkAudio::Layout    m_adevLayout;
    double              m_adevRate;
    size_t              m_adevSamples;
    size_t              m_adevBufferWindow;
    mutable QReadWriteLock m_mediaMutex{QReadWriteLock::Recursive};
    Time                m_grainDuration;
    Time                m_grainEnvelope;
    Components          m_tempComponents;
    static bool         m_sourceNameInID;
    Parameters          m_inparams;
    std::atomic< IPGraph::WorkItemID > 
                        m_workItemID;
    Mutex               m_dispatchIDListMutex;
    std::list< Application::DispatchID >
                        m_dispatchIDList;
    std::atomic< IPGraph::WorkItemID > 
                        m_loadingID;
    Mutex               m_dispatchIDCancelRequestedMutex;
    std::set< Application::DispatchID > 
                        m_dispatchIDCancelRequestedSet;
    bool                m_progressiveSourceLoading;
    std::atomic< int >  m_jobsCancelling {0};
};

} // Rv

#endif // __IPGraph__FileSourceIPNode__h__
