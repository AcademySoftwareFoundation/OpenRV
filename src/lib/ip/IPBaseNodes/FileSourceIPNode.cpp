//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/FBCache.h>
#include <TwkApp/Bundle.h>
#include <TwkApp/Event.h>
#include <TwkAudio/Mix.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/GTOWriter.h>
#include <TwkFB/Operations.h>
#include <TwkFBAux/FBAux.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Mat44.h>
#include <TwkMovie/ResamplingMovie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/MovieReader.h>
#include <TwkContainer/GTOReader.h>
#include <TwkUtil/Base64.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkMediaLibrary/Library.h>

#include <algorithm>
#include <iterator>

#include <boost/algorithm/string/regex.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>

#define AUDIO_READPOSITIONOFFSET_THRESHOLD \
  0.01  // In secs; this is the max amount of slip we will allow

#define AUDIO_RESAMPLER_WINDOW_SIZE 64

#define PROFILE_SAMPLE( P, X )                                           \
  if( P )                                                                \
  {                                                                      \
    IPGraph::EvalProfilingRecord& r = graph()->currentProfilingSample(); \
    r.X = graph()->profilingElapsedTime();                               \
  }

static ENVVAR_BOOL( evIgnoreAudio, "RV_IGNORE_AUDIO", false );
static ENVVAR_BOOL( evDebugCookies, "RV_DEBUG_FFMPEG_COOKIES", false );
static ENVVAR_BOOL( evDebugHeaders, "RV_DEBUG_FFMPEG_HEADERS", false );

namespace IPCore
{
  using namespace std;
  using namespace TwkMovie;
  using namespace TwkMath;
  using namespace TwkFB;
  using namespace TwkContainer;
  using namespace TwkAudio;
  using namespace boost;
  using namespace TwkApp;

  float FileSourceIPNode::defaultFPS = 24.0;
  float FileSourceIPNode::defaultOverrideFPS = 0.0;
  bool FileSourceIPNode::m_sourceNameInID = true;

  namespace
  {
    size_t fallbackCacheSize = 0;
    float allocTimeTotal = 0.0;
    float allocTimeCount = 0.0;
    float allocTimeMin = FLT_MAX;
    float allocTimeMax = FLT_MIN;

    unsigned char blankImage[4] = { 0, 0, 0, 0 };
    unsigned char blackImage[4] = { 0, 0, 0, 1 };

  }  // namespace

  namespace
  {

    Mat44f orientationMatrix( FrameBuffer::Orientation o )
    {
      Mat44f T;

      if( o == FrameBuffer::TOPLEFT || o == FrameBuffer::TOPRIGHT )
      {
        Mat44f S;
        S.makeScale( Vec3f( 1, -1, 1 ) );
        T *= S;
      }

      if( o == FrameBuffer::TOPRIGHT || o == FrameBuffer::BOTTOMRIGHT )
      {
        Mat44f S;
        S.makeScale( Vec3f( -1, 1, 1 ) );
        T *= S;
      }

      return T;
    }

    FrameBuffer* allocateResizedFBs( FrameBuffer* in, float sx, float sy,
                                     FrameBuffer::DataType outType )
    {
      int ow = int( float( in->width() ) * sx );
      int oh = int( float( in->height() ) * sy );

      if( ow != in->width() || oh != in->height() || outType != in->dataType() )
      {
        FrameBuffer* fb =
            new FrameBuffer( ow, oh, in->numChannels(), outType, 0,
                             &in->channelNames(), in->orientation() );

        if( in->nextPlane() )
          fb->appendPlane(
              allocateResizedFBs( in->nextPlane(), sx, sy, outType ) );
        return fb;
      }
      else
      {
        return in;
      }
    }

    FrameBuffer* resizeFB( FrameBuffer* inFB, float scale )
    {
      FrameBuffer* outFB =
          allocateResizedFBs( inFB, scale, scale, inFB->dataType() );
      TwkFBAux::resize( inFB, outFB );
      outFB->setIdentifier( inFB->identifier() );
      inFB->copyAttributesTo( outFB );
      return outFB;
    }

    int getLengthLimit()
    {
      // Check if the user set an override on the maximum texture size.
      int lengthLimit =
          Application::optionValue<int>( "maxTextureSizeOverride", int( 0 ) );
      // If the user did not set an override, honor the real setting, defaulting
      // to 8192 if none.
      if( lengthLimit == 0 )
      {
        lengthLimit =
            Application::optionValue<int>( "maxTextureSize", int( 8192 ) );
      }

      return lengthLimit;
    }

    struct TilingInfo
    {
      float scale{ 1.0 };
      int numTiles{ 1 };
    };

    const bool DEBUG_ANIMATED_TILING = getenv( "RV_DEBUG_ANIMATED_TILING" );
    const bool SOURCE_TILING = getenv( "RV_SOURCE_TILING" );

    TilingInfo getTilingInfo( const int fullFBWidth, const int fullFBHeight )
    {
      TilingInfo tilingInfo;

      const int lengthLimit = getLengthLimit();

      if( !SOURCE_TILING )
      {
        // When source-tiling is OFF, scale down the source texture to the
        // maximum texture size (lengthLimit) if required.
        const int maxLength = std::max( fullFBWidth, fullFBHeight );
        if( maxLength > lengthLimit )
        {
          tilingInfo.scale = float( lengthLimit ) / float( maxLength );
        }
      }
      else
      {
        // When source-tiling is ON, compute the smallest square layout that
        // contains square tiles that are equal-or-less-than the maximum texture
        // size (lengthLimit).
        const int nbTilesX =
            static_cast<int>( ceil( static_cast<float>( fullFBWidth ) /
                                    static_cast<float>( lengthLimit ) ) );
        const int nbTilesY =
            static_cast<int>( ceil( static_cast<float>( fullFBHeight ) /
                                    static_cast<float>( lengthLimit ) ) );
        tilingInfo.numTiles = std::max( nbTilesX, nbTilesY );
      }

      return tilingInfo;
    }

  }  // namespace

  struct FileSourceIPNode::SharedMedia
  {
    SharedMedia( bool initHasValidRange )
        : privateContainer( 0 ),
          hasVideo( false ),
          hasAudio( false ),
          hasValidRange( initHasValidRange )
    {
    }
    ~SharedMedia()
    {
      TwkApp::Bundle* bundle = TwkApp::Bundle::mainBundle();
      delete privateContainer;
    }

    SharedMoviePointerVector movies;
    SharedMoviePointer audioMovie;
    PropertyContainer* privateContainer;
    StringSet views;
    StringSet layers;
    StringSet channels;
    bool hasVideo;
    bool hasAudio;
    bool hasValidRange;

    Movie* primaryMovie() const
    {
      return movies.empty() ? 0 : movies.front().get();
    }
    Movie* primaryAudioMovie() const
    {
      return audioMovie.get() ? audioMovie.get() : primaryMovie();
    }

    bool hasView( const std::string& viewName ) const
    {
      return views.count( viewName ) > 0;
    }
    bool hasLayer( const std::string& layerName ) const
    {
      return layers.count( layerName ) > 0;
    }

    bool hasChannel( const std::string& channelName ) const
    {
      return channels.count( channelName ) > 0;
    }

    bool matchesView( const ImageComponent& i ) const
    {
      return i.name.size() >= 1 && hasView( i.name[0] );
    }

    bool matchesLayer( const ImageComponent& i ) const
    {
      return i.name.size() >= 2 && hasLayer( i.name[1] );
    }

    bool matchesChannel( const ImageComponent& i ) const
    {
      return i.name.size() >= 3 && hasChannel( i.name[2] );
    }

    Movie* movieForThread( size_t index ) const
    {
      return movies.size() > index ? movies[index].get() : movies.front().get();
    };
  };

  class FileSourceIPNode::Media : public ResamplingMovie
  {
   public:
    Media( const SharedMediaPointer& sharedMedia )
        : ResamplingMovie( sharedMedia->primaryAudioMovie() ),
          shared( sharedMedia )
    {
    }

    ~Media() {}

    bool hasVideo() const
    {
      return shared->hasVideo;
    }
    bool hasAudio() const
    {
      return shared->hasAudio;
    }
    bool hasView( const std::string& v ) const
    {
      return shared->hasView( v );
    }
    bool hasLayer( const std::string& l ) const
    {
      return shared->hasLayer( l );
    }
    bool hasChannel( const std::string& c ) const
    {
      return shared->hasChannel( c );
    }
    bool matchesView( const ImageComponent& i ) const
    {
      return shared->matchesView( i );
    }
    bool matchesLayer( const ImageComponent& i ) const
    {
      return shared->matchesLayer( i );
    }
    bool matchesChannel( const ImageComponent& i ) const
    {
      return shared->matchesChannel( i );
    }
    Movie* primaryMovie() const
    {
      return shared->movies.empty() ? 0 : shared->movies.front().get();
    }
    Movie* audioMovie() const
    {
      return shared->primaryAudioMovie();
    }

    std::string viewName;
    size_t index;
    SharedMediaPointer shared;
  };

  FileSourceIPNode::FileSourceIPNode( const std::string& name,
                                      const NodeDefinition* def, IPGraph* g,
                                      GroupIPNode* group,
                                      const std::string mediaRepName )
      : SourceIPNode( name, def, g, group, mediaRepName ), m_workItemID( 0 )
  {
    setMaxInputs( 0 );
    setMinInputs( 0 );

    PropertyInfo* ainfo =
        new PropertyInfo( PropertyInfo::ExcludeFromProfile |
                          PropertyInfo::Persistent | PropertyInfo::Animatable );

    PropertyInfo* einfo = new PropertyInfo( PropertyInfo::ExcludeFromProfile |
                                            PropertyInfo::Persistent |
                                            PropertyInfo::RequiresGraphEdit );

    m_offset = 0;
    m_rangeOffset = 0;
    m_rangeStart = 0;
    m_noMovieAudio = 0;
    m_volume = 0;
    m_balance = 0;
    m_adevRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
    m_adevSamples = 512;
    m_adevLayout = TwkAudio::Stereo_2;
    m_adevBufferWindow = m_adevSamples * 10;
    m_grainDuration = 0.022;
    m_grainEnvelope = 0.006;
    m_mediaMovies = declareProperty<StringProperty>( "media.movie" );
    m_mediaViewNames = declareProperty<StringProperty>( "media.name" );
    m_fps = declareProperty<FloatProperty>( "group.fps", 0.0f, einfo );
    m_cutIn = declareProperty<IntProperty>(
        "cut.in", -numeric_limits<int>::max(), einfo );
    m_cutOut = declareProperty<IntProperty>(
        "cut.out", numeric_limits<int>::max(), einfo );
    m_volume = declareProperty<FloatProperty>( "group.volume", 1.0f, ainfo );
    m_offset =
        declareProperty<FloatProperty>( "group.audioOffset", 0.0f, ainfo );
    m_rangeOffset = declareProperty<IntProperty>( "group.rangeOffset", 0 );
    m_noMovieAudio =
        declareProperty<IntProperty>( "group.noMovieAudio", 0, ainfo );
    m_balance = declareProperty<FloatProperty>( "group.balance", 0.0f, ainfo );
    m_crossover =
        declareProperty<FloatProperty>( "group.crossover", 0.0f, ainfo );
    m_readAllChannels =
        declareProperty<IntProperty>( "request.readAllChannels", 0 );
    m_rangeStart = 0;

    const bool progressiveSourceLoading =
        Application::optionValue<bool>( "progressiveSourceLoading", false );
    setProgressiveSourceLoading( progressiveSourceLoading );
  }

  FileSourceIPNode::~FileSourceIPNode()
  {
    cancelJobs();
    clearMedia();
  }

  FileSourceIPNode::SharedMedia* FileSourceIPNode::newSharedMedia(
      Movie* mov, bool hasValidRange )
  {
    SharedMedia* sharedMedia = new SharedMedia( hasValidRange );
    const MovieInfo& info = mov->info();

    sharedMedia->hasVideo = mov->hasVideo();
    sharedMedia->hasAudio = mov->hasAudio();
    sharedMedia->privateContainer =
        reinterpret_cast<PropertyContainer*>( mov->info().privateData );

    //
    //  First movie is used by the display thread (thread 0)
    //

    SharedMoviePointer movp( mov );
    sharedMedia->movies.push_back( movp );

    if( MovieReader* reader = dynamic_cast<MovieReader*>( mov ) )
    {
      if( !reader->isThreadSafe() )
      {
        //
        //  Audio gets its own copy.
        //

        if( sharedMedia->hasAudio )
        {
          SharedMoviePointer movp( reader->clone() );
          sharedMedia->audioMovie = movp;
        }

        const size_t nthreads =
            graph()->numEvalThreads() + 1;  //  rthreads + display thread

        //
        //  Each reader thread gets its own copy.
        //

        while( sharedMedia->movies.size() < nthreads )
        {
          if( Movie* clone = reader->clone() )
          {
            SharedMoviePointer movp( clone );
            sharedMedia->movies.push_back( movp );
          }
          else
          {
            cerr << "ERROR: failed to clone Movie '" << reader->filename()
                 << "'" << endl;
            break;
          }
        }
      }
    }

    //
    //  Add the new view names to the set of view names.
    //

    if( !sharedMedia->movies.empty() )
    {
      for( size_t q = 0; q < info.views.size(); q++ )
      {
        const string& vname = info.views[q];
        sharedMedia->views.insert( vname );
      }

      if( info.viewInfos.size() == 0 )
      {
        for( size_t q = 0; q < info.layers.size(); q++ )
        {
          const string& lname = info.layers[q];
          sharedMedia->layers.insert( lname );
        }
        for( size_t q = 0; q < info.channelInfos.size(); q++ )
        {
          sharedMedia->channels.insert( info.channelInfos[q].name );
        }
      }
      else
      {
        //
        // This allows use to match a view-layer-channel
        // request against the media later on in mediaByRequest.
        //

        for( size_t vi = 0; vi < info.viewInfos.size(); vi++ )
        {
          const FBInfo::ViewInfo& vinfo = info.viewInfos[vi];
          const string& vname = vinfo.name;
          sharedMedia->views.insert( vname );

          for( size_t li = 0; li < vinfo.layers.size(); li++ )
          {
            const FBInfo::LayerInfo& linfo = vinfo.layers[li];
            sharedMedia->layers.insert( linfo.name );

            for( size_t ci = 0; ci < linfo.channels.size(); ci++ )
            {
              const FBInfo::ChannelInfo& cinfo = linfo.channels[ci];
              sharedMedia->channels.insert( cinfo.name );
            }
          }

          // Handle non-layered channels
          if( vinfo.otherChannels.size() )
          {
            //  Make sure we have a default layer for the channels
            sharedMedia->layers.insert( "" );

            for( size_t ci = 0; ci < vinfo.otherChannels.size(); ci++ )
            {
              const FBInfo::ChannelInfo& cinfo = vinfo.otherChannels[ci];
              sharedMedia->channels.insert( cinfo.name );
            }
          }
        }
      }
    }

    return sharedMedia;
  }

  void FileSourceIPNode::changeMedia(
      const SharedMediaPointer& sharedMedia,
      const SharedMediaPointer& proxySharedMedia )
  {
    const QWriteLocker writeLock( &m_mediaMutex );

    Media* media = new Media( sharedMedia );
    MediaPointer mediap( media );

    if( proxySharedMedia )
    {
      for( auto i = 0; i < m_mediaVector.size(); i++ )
      {
        if( m_mediaVector[i]->shared == proxySharedMedia )
        {
          mediap->index = i;
          m_mediaVector[i] = mediap;
          break;
        }
      }
    }
    else
    {
      m_mediaVector.push_back( mediap );
      media->index = m_mediaVector.size() - 1;

      //
      //  The view name is used for hashing
      //

      ostringstream vstr;
      vstr << media->index;
      media->viewName = vstr.str();
    }

    //
    // Recalculate the state of the media
    //
    bool hasVideo = false;
    bool hasAudio = false;
    m_allViews.clear();
    m_viewNameSet.clear();

    for( auto media : m_mediaVector )
    {
      if( media->hasVideo() ) hasVideo = true;

      if( media->hasAudio() &&
          ( !media->hasVideo() || !m_noMovieAudio->front() ) )
        hasAudio = true;

      Movie* mov = media->primaryMovie();
      const MovieInfo& info = mov->info();

      for( size_t q = 0; q < info.views.size(); q++ )
      {
        const string& vname = info.views[q];

        if( m_viewNameSet.count( vname ) == 0 )
        {
          m_allViews.push_back( vname );
          m_viewNameSet.insert( vname );
        }
      }

      if( !info.viewInfos.empty() )
      {
        //
        // This allows use to match a view-layer-channel
        // request against the media later on in mediaByRequest.
        //

        for( size_t vi = 0; vi < info.viewInfos.size(); vi++ )
        {
          const FBInfo::ViewInfo& vinfo = info.viewInfos[vi];
          const string& vname = vinfo.name;

          if( m_viewNameSet.count( vname ) == 0 )
          {
            m_allViews.push_back( vname );
            m_viewNameSet.insert( vname );
          }
        }
      }
    }

    setHasVideo( hasVideo );
    setHasAudio( evIgnoreAudio.getValue() ? false : hasAudio );
    updateStereoViews( m_allViews, m_eyeViews );

    if( hasAudio )
    {
      AudioConfiguration config( m_adevRate, m_adevLayout, m_adevSamples );
      audioConfigure( config );
    }

    if( defaultOverrideFPS != 0 )
    {
      m_fps->front() = defaultOverrideFPS;
    }
  }

  void FileSourceIPNode::addMedia( const SharedMediaPointer& sharedMedia,
                                   const SharedMediaPointer& proxySharedMedia )
  {
    HOP_PROF_FUNC();

    if( m_jobsCancelling )
    {
      // we are on the middle of a job cancellation
      return;
    }

    // Dispatch to main thread only when using async loading.
    // In sync mode, we do not want to dispatch the events, because that
    // would postpones their execution and may lead to wrong behavior.
    // e.g. propagateMediaChange() might occurs before those two.
    if( m_workItemID )
    {
      addDispatchJob( Application::instance()->dispatchToMainThread(
          [this, sharedMedia, proxySharedMedia]( Application::DispatchID dispatchID )
          {
            {
              LockGuard dispatchGuard( m_dispatchIDCancelRequestedMutex );

              bool isCanceled =
                  ( m_dispatchIDCancelRequestedSet.count( dispatchID ) >= 1 );
              if( isCanceled )
              {
                m_dispatchIDCancelRequestedSet.erase( dispatchID );
                return;
              }
            }

            {
              const QWriteLocker writeLock( &m_mediaMutex );
              changeMedia( sharedMedia, proxySharedMedia );
              updateHasAudioStatus();
            }

            // The order of propagation is important. The AudioStatus event must
            // be send the first one. After we must send MediaChange event to
            // notify a FPS change before send ImageStructureChange and
            // rangeChange. This new fps is used by layout trigges by the
            // RangeChange event
            propagateMediaChange();
            propagateImageStructureChange();
            propagateRangeChange();

            {
              LockGuard dispatchGuard( m_dispatchIDCancelRequestedMutex );
              bool isCanceled =
                  ( m_dispatchIDCancelRequestedSet.count( dispatchID ) >= 1 );
              if( isCanceled )
              {
                m_dispatchIDCancelRequestedSet.erase( dispatchID );
                return;
              }
            }

            removeDispatchJob( dispatchID );
          } ) );
    }
    else
    {
      {
        const QWriteLocker writeLock( &m_mediaMutex );
        changeMedia( sharedMedia, proxySharedMedia );
        updateHasAudioStatus();
      }

      // See previous comment
      propagateMediaChange();
      propagateImageStructureChange();
      propagateRangeChange();
    }
  }

  void FileSourceIPNode::testEvaluate( const Context& context,
                                       TestEvaluationResult& result )
  {
    bool slow = false;

    for( size_t i = 0, s = numMedia(); i < s; i++ )
    {
      const MovieInfo& info = mediaMovieInfo( i );
      if( info.slowRandomAccess && info.video ) slow = true;
    }

    result.poorRandomAccessPerformance =
        slow || result.poorRandomAccessPerformance;
  }

  FileSourceIPNode::MediaPointer FileSourceIPNode::getMediaFromContext(
      ImageComponent& selection, const Context& context )
  {
    selection = selectComponentFromContext( context );

    MediaPointer media = mediaForComponent( selection, context );
    if( context.stereo )
    {
      ImageComponent newSelection = stereoComponent( selection, context.eye );

      if( newSelection.isValid() )
      {
        if( MediaPointer newMedia = mediaForComponent( newSelection, context ) )
        {
          selection = newSelection;
          media = newMedia;
        }
      }
    }

    if( !selection.isValid() )
    {
      media = defaultMedia( context.stereo ? context.eye : 0 );
    }

    return media;
  }

  FileSourceIPNode::MediaPointer FileSourceIPNode::mediaForComponent(
      ImageComponent& c, const Context& context )
  {
    if( !c.isValid() ) return nullptr;

    //
    //  Search for an exact match
    //

    const QReadLocker readLock( &m_mediaMutex );

    if( context.stereo && context.eye < m_mediaVector.size() )
    {
      //
      // This implies the stereo rendering case where we have loaded
      // left and right media; so we ONLY match c against the media
      // in the list thats indexed by the value of context.eye.
      //
      // If we don't find a match, we will attempt another match
      // across all the media later on in this function.
      //
      //  e.g. rv [ left.exr right.exr ]
      //
      // NB: Both left.exr and right.exr can be without view names
      //     but layered which is why we have to restrict the search
      //     to the media in the list indexed by context.eye's
      //     value.
      //

      auto media = m_mediaVector[context.eye];
      if( media->hasVideo() )
      {
        switch( c.type )
        {
          default:
            break;
          case ChannelComponent:
            if( !media->matchesChannel( c ) ) break;
          case LayerComponent:
            if( !media->matchesLayer( c ) ) break;
          case ViewComponent:
            if( !media->matchesView( c ) ) break;
            return media;
        }
      }
    }

    // For non-stereo case matching and if the stereo case match above
    // didnt yield a result.
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      auto media = m_mediaVector[i];

      if( media->hasVideo() )
      {
        switch( c.type )
        {
          default:
            break;
          case ChannelComponent:
            if( !media->matchesChannel( c ) ) break;
          case LayerComponent:
            if( !media->matchesLayer( c ) ) break;
          case ViewComponent:
            if( !media->matchesView( c ) ) break;
            return media;
        }

        // If we got here; we havent been able to match any media with
        // our component selection values. So we will check for the
        // case where the view is empty and determine what the media's
        // defaultView is and use that as the view string instead of ""
        // which implies defaultView anyhow.
        if( c.type == LayerComponent )
        {
          if( c.name[0].empty() )
          {
            // This handles the case where the view has been omitted in the
            // session mgr and therefore is "". This happens when there is just
            // one view for all parts/layers. So we need to determine what the
            // defaultView is to use as the view string when attempting to match
            // the media. Note that if we are this point in the code, all prior
            // media match attempts have failed.
            //
            if( Movie* mov = movieForThread( media.get(), context ) )
            {
              ImageComponent newSelection = ImageComponent(
                  LayerComponent, mov->info().defaultView, c.name[1] );
              if( media->matchesLayer( newSelection ) )
              {
                c = newSelection;
                return media;
              }
            }
          }
        }

        if( c.type == ChannelComponent )
        {
          // This handles the case where the view has been omitted in the
          // session mgr and therefore is "". This happens when there is just
          // one view for all parts/layers. So we need to determine what the
          // defaultView is to use as the view string when attempting to match
          // the media. Note that if we are this point in the code, all prior
          // media match attempts have failed.
          //
          if( Movie* mov = movieForThread( media.get(), context ) )
          {
            ImageComponent newSelection =
                ImageComponent( ChannelComponent, mov->info().defaultView,
                                c.name[1], c.name[2] );
            if( media->matchesChannel( newSelection ) )
            {
              c = newSelection;
              return media;
            }
          }
        }
      }
    }

    return nullptr;
  }

  FileSourceIPNode::MediaPointer FileSourceIPNode::defaultMedia( int eye )
  {
    const QReadLocker readLock( &m_mediaMutex );

    int count = 0;

    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      auto media = m_mediaVector[i];

      if( media->hasVideo() )
      {
        if( count == eye ) return media;
        count++;
      }
    }

    //
    //  if we didn't find the eye and its not eye==0 just get eye 0
    //

    if( eye > 0 ) return defaultMedia( 0 );

    //
    //  No media found. Presumably if we have an audio media we should just
    //  return that.
    //

    return m_mediaVector.empty() ? nullptr : m_mediaVector.front();
  }

  FileSourceIPNode::Movie* FileSourceIPNode::movieForThread(
      const Media* media, const Context& context )
  {
    if( !media ) return 0;
    return media->shared->movieForThread( context.threadNum );
  }

  void FileSourceIPNode::setupRequest( const Movie* mov,
                                       const ImageComponent& selection,
                                       const Context& context,
                                       Movie::ReadRequest& request )
  {
    //
    //  Assemble request frame: rangeOffset shifts the frame range, rangeStart
    //  forces an absolute start frame.
    //

    if( m_rangeOffset && m_rangeOffset->size() )
    {
      request.frame = request.frame - m_rangeOffset->front();
    }

    if( m_rangeStart && m_rangeStart->size() )
    {
      request.frame = request.frame - m_rangeStart->front() + mov->info().start;
    }

    request.missing = context.missing;
    request.allChannels = ( m_readAllChannels->front() ? true : false );

    //
    //  Limit requested views to ones this movie actually provides.
    //  Otherwise the movie reader may fallback to a "default" view,
    //  even though a later movie is going to provide the "real" left
    //  view or whatever.
    //

    request.views.resize( 0 );
    request.layers.resize( 0 );
    request.channels.resize( 0 );

    const size_t s = selection.name.size();
    const ComponentType t = selection.type;

    if( selection.type != NoComponent )
    {
      if( s > 0 && t >= ViewComponent )
        request.views.push_back( selection.name[0] );
      if( s > 1 && t >= LayerComponent )
        request.layers.push_back( selection.name[1] );
      if( s > 2 && t == ChannelComponent )
        request.channels.push_back( selection.name[2] );
    }

    //
    //  Copy the input parameters
    //

    std::copy( m_inparams.begin(), m_inparams.end(),
               back_inserter( request.parameters ) );
  }

  namespace
  {

    static void wait( int sleepMS )
    {
#ifdef PLATFORM_WINDOWS
      Sleep( sleepMS );
#else
      usleep( 1000 * sleepMS );
#endif
    }

    //
    //  A convenience routine to make each frame take longer to read, to test
    //  caching, etc.
    //

    static void debuggingDelay()
    {
      static int sleepMS = -1;

      if( sleepMS == -1 )
      {
        if( getenv( "TWK_SOURCE_SLEEP_MS" ) )
          sleepMS = atoi( getenv( "TWK_SOURCE_SLEEP_MS" ) );
        else
          sleepMS = 0;
      }
      if( sleepMS > 0 )
      {
        wait( sleepMS );
      }
    }

    static void debuggingLoadDelay()
    {
      static int sleepMS = -1;

      if( sleepMS == -1 )
      {
        if( getenv( "RV_SOURCE_LOAD_DELAY_MS" ) )
          sleepMS = atoi( getenv( "RV_SOURCE_LOAD_DELAY_MS" ) );
        else
          sleepMS = 0;
      }
      if( sleepMS > 0 )
      {
        wait( sleepMS );
      }
    }

  };  // namespace

  void FileSourceIPNode::configureAlphaAttrs( FrameBuffer* fb, IPImage* img )
  {
    // Configure a FrameBuffer and an IPImage so they use the right alpha and
    // {un}premultiplied settings.
    bool hasAlphaAttr = fb->hasAttribute( "AlphaType" );
    img->unpremulted = false;

    if( !hasAlphaAttr && ( fb->numChannels() == 2 || fb->numChannels() == 4 ) )
    {
      fb->attribute<string>( "AlphaType" ) = "Premultiplied";
    }
    else if( hasAlphaAttr )
    {
      img->unpremulted =
          fb->attribute<string>( "AlphaType" ) == "Unpremultiplied";
    }
  }

  IPImage* FileSourceIPNode::evaluate( const Context& context )
  {
    // Make sure to prioritize this source. If the source is already loaded
    // or is currently loading, this will have no effect.
    graph()->prioritizeWorkItem( m_workItemID );

    const bool profile =
        ( context.thread & DisplayThread ) && graph()->needsProfilingSamples();
    ImageComponent selection;
    MediaPointer media;
    {
      const QReadLocker readLock( &m_mediaMutex );

      // NOTE: Might be loading still
      if( m_mediaVector.size() == 0 )
        return IPImage::newNoImage( this, "No Media" );

      media = getMediaFromContext( selection, context );
    }

    Movie* mov = movieForThread( media.get(), context );
    if( !mov || !media )
    {
      TWK_THROW_EXC_STREAM(
          "ERROR: FileSourceIPNode::evaluate: no media found." );
    }

    Movie::ReadRequest request( context.frame, context.stereo );
    setupRequest( mov, selection, context, request );

    //
    //  Call the movie evaluate
    //

    FrameBufferVector fbs;
    bool failed = false;
    bool empty = false;
    string errorString;

    PROFILE_SAMPLE( profile, ioStart );

    debuggingDelay();

    try
    {
#if defined( HOP_ENABLED )
      std::string imagesAtFrameMsg = std::string( "imagesAtFrame(" ) +
                                     std::to_string( context.frame ) +
                                     std::string( ") : " ) + filename().c_str();
      HOP_PROF_DYN_NAME( imagesAtFrameMsg.c_str() );
#endif

      mov->imagesAtFrame( request, fbs );

      if( fbs.empty() )
      {
        empty = true;
        if( media->hasVideo() )
        {
          //
          //  A well written reader should throw on an error or
          //  return a partial fb if it only partially succeeded. In
          //  the case that the reader code is broken we might get
          //  this: nothing.

          errorString = "Reader appears to be broken";
        }
      }
    }
    catch( std::exception& exc )
    {
      failed = true;
      fbs.clear();
      errorString = exc.what();
    }
    catch( ... )
    {
      failed = true;
      fbs.clear();
      errorString = "Unknown Exception";
    }

    PROFILE_SAMPLE( profile, ioEnd );

    if( failed || empty )
    {
      Movie::IdentifierVector ids;

      //
      //  Do this here so that readers don't have to be smart about
      //  this situation. Basically make a bunch of blank images
      //  with the correct ids. Later in the function these will be
      //  marked as "missing" via an image attribute.
      //

      try
      {
        request.missing = false;
        mov->identifiersAtFrame( request, ids );
      }
      catch( ... )
      {
        //
        //  Try again, allow for nearby frames.
        //
        //  Otherwise we can have the situation where a frame is missing,
        //  so we try for a nearby frame, but that is corrupt or incomplete
        //  so the read fails, and we end up here.  The error image/fb
        //  should have the id of the nearby frame.
        //
        try
        {
          request.missing = true;
          mov->identifiersAtFrame( request, ids );
        }
        catch( ... )
        {
          //  This should never happen.
          cerr << "ERROR: FileSource: identifiersAtFrame error: unable to get "
                  "any identifiers from movie."
               << endl;
        }
      }
      if( ids.size() == 0 )
      {
        if( media->hasVideo() )
        {
          //  This should never happen, but if it does still make sure that fbs
          //  array is not empty.
          cerr << "ERROR: FileSource eval: unable to get any identifiers from "
                  "movie."
               << endl;
        }
        ids.push_back( "no-source-image" );
      }

      while( fbs.size() < ids.size() )
      {
        FrameBuffer* fb = IPImage::newNoImageFrameBufferWithAttrs(
            this, mov->info().width, mov->info().height, "No Source Image" );
        fbs.push_back( fb );
      }

      for( size_t i = 0; i < ids.size(); i++ )
      {
        FrameBuffer* fb = fbs[i];
        fb->setIdentifier( "" );
        fb->idstream() << ids[i];
      }
    }

    //
    //  Before unlocking: make sure we don't accidently "share" pixels
    //  with another thread. If not here, this will happen at the
    //  cache anyway. The only movie object that seems to require this
    //  is Apple Quicktime.
    //
    //  Also, delete any additional FrameBuffers the reader decided to
    //  give us for some reason.
    //

    if( !fbs.empty() )
    {
      fbs.front()->ownData();
      for( size_t i = 1; i < fbs.size(); i++ ) delete fbs[i];
      fbs.resize( 1 );
    }

    //
    //  If this is a caching thread, and the cache became full wile we
    //  were doing IO, then the caching threads will be in the process
    //  of stopping.  In that case, abort the evaluation chain (and
    //  eventual caching) at this point.
    //
    //  _Unless_ this fb is going into the per-node cache (context.cacheNode
    //  != 0), in which case we allow it to continue, since the size of that
    //  cache is not restricted.
    //

    if( CacheEvalThread == context.thread && !graph()->cacheThreadContinue() &&
        !context.cacheNode )
    {
      //
      //  We want to be sure the cache is actually overflowing before we
      //  throw these fbs away, but we also don't want to lock unless we have
      //  to.
      //

      graph()->cache().lock();
      bool overflowing = graph()->cache().overflowing();
      graph()->cache().unlock();

      if( overflowing )
      {
        //
        //  We own these frame buffers, so delete them or we have a
        //  memory leak.
        //

        if( !fbs.empty() ) delete fbs.front();
        throw CacheFullExc();
      }
    }

    //
    //  If we're supposed to provide stereo and there's only one eye
    //  in this layer and this is the only layer, duplicate the eye.
    //

    FrameBuffer* fullFB = fbs[0];

    // This only needs to be performed on the fullFB, since tileFBs will be
    // duplicates of fullFB.
    addUserAttributes( fullFB );

    fullFB->idstream() << idFromAttributes();

    if( m_sourceNameInID ) fullFB->idstream() << "/" << name();

    fullFB->idstream() << "." << media->index << "/";

    //
    //  If there's no Eye attr the first and second images become the
    //  left and right eyes.
    //

    bool leftEye = context.stereo ? context.eye == 0 : true;
    bool rightEye = context.stereo ? context.eye == 1 : true;

    int eye = 0;
    if( leftEye ) eye |= 1;
    if( rightEye ) eye |= 1 << 1;
    IntAttribute* a = fullFB->newAttribute<int>( "Eye", eye );

    ostringstream sourceValue;

    const StringAttribute* va =
        dynamic_cast<const StringAttribute*>( fullFB->findAttribute( "View" ) );

    int lframe = request.frame;
    if( lframe > mov->info().end ) lframe = mov->info().end;
    if( lframe < mov->info().start ) lframe = mov->info().start;

    fullFB->newAttribute<int>( "SourceFrame", lframe );

    sourceValue << name() << "."
                << ( ( context.stereo && context.eye == 1 ) ? 1 : 0 ) << "/"
                << ( va ? va->value() : "0" ) << "/" << lframe;

    TilingInfo tilingInfo = getTilingInfo( fullFB->width(), fullFB->height() );

    if( tilingInfo.scale < 1.0f )
    {
      FrameBuffer* scaledFB = resizeFB( fullFB, tilingInfo.scale );
      scaledFB->idstream() << "*" << tilingInfo.scale;
      delete fullFB;
      fullFB = scaledFB;
    }

    // For debugging, use different tiling layouts based on frame number:
    // frame 1 : 1x1
    // frame 2 : 2x2
    // etc.
    // Note : this debug mode is only enabled when there should not be tiling.
    if( tilingInfo.numTiles == 1 && DEBUG_ANIMATED_TILING )
    {
      tilingInfo.numTiles = context.frame;
    }

    // 'root' can either be the full IPImage (when tiling is off) or a collage
    // of tiled IPImages (when tiling is on).
    IPImage* root = nullptr;

    if( tilingInfo.numTiles == 1 )
    {
      root = new IPImage( this, IPImage::BlendRenderType, fullFB );
    }
    else
    {
      root = new IPImage( this, IPImage::BlendRenderType, fullFB->width(),
                          fullFB->height(), 1.0 );
    }

    if( failed )
    {
      root->missing = true;
      root->missingLocalFrame = request.frame;
      fullFB->clearAttributes();
      root->setErrorState( this, errorString );
    }

    fullFB->attribute<string>( "RVSource" ) = sourceValue.str();

    // Configure the root FrameBuffer/IPImage.
    configureAlphaAttrs( fullFB, root );

    if( tilingInfo.numTiles == 1 )
    {
      root->info = &( mov->info() );

      root->shaderExpr = Shader::sourceAssemblyShader( root );
      root->recordResourceUsage();
      return root;
    }
    else
    {
      // This IPImage has a size that does not fit in a texture, tag it
      // so it is never rendered as an intermediate.
      root->noIntermediate = true;

      ImageStructureInfo info = imageStructureInfo( context );

      FrameBuffer* masterBuffer( nullptr );
      std::vector<FrameBuffer*> proxyBuffers;

      //  Conceptual structure of a 2x2 tiling layout with a 3-planes buffer
      //
      //       B _____________
      //     G __|_____|____ |
      //   R __|_____|____ | |
      //     |     |     | |_|
      //     |  Pr |  Pr |_| |
      //     |_____|_____| | |
      //     |     |     | |_|
      //     |  Mr |  Pr |_|
      //     |_____|_____|
      //
      //  R/G/B : Channel planes (red, green, blue)
      //  Mx : Master Tile (a FrameBuffer) for plane x (eg.: R,G,B)
      //  Px : Proxy Tile (a FrameBuffer) for plane x (eg.: R,G,B)
      //
      //  * The 3 level of planes are for R/G/B channels.
      //
      //  * Each tile has a pointer to the next
      //    (eg.: Mr-->Mg-->Mb, Pr-->Pg-->Pb).
      //
      //  * Only the first plane (R in this example) is put in the cache and
      //    it "total size" includes sub-planes.
      //
      for( int y = 0; y < tilingInfo.numTiles; y++ )
      {
        for( int x = 0; x < tilingInfo.numTiles; x++ )
        {
          FrameBuffer* tileFB = new FrameBuffer();

          // This will copy all attributes and planes (without allocating
          // new pixel buffers).
          tileFB->shallowCopy( fullFB );

          // We use the first tile FB as the master FB and the other
          // tile FB are just proxy buffers, ie.: they refer to a section
          // of the master buffer.
          const bool isMasterBuffer = x == 0 && y == 0;

          if( isMasterBuffer )
          {
            masterBuffer = tileFB;
          }
          else
          {
            proxyBuffers.push_back( tileFB );

            // Flag the proxy buffers (non-first tiles) by passing
            // them their master buffer pointer (owner) so they
            // can notify it when they get evicted.
            // This can be performed on the first plane since only the
            // first plane is explicitely put in the cache. Sub-planes
            // are implicitely put in the cache (planes are chained).
            tileFB->attribute<FrameBuffer*>( "ProxyBufferOwnerPtr" ) =
                masterBuffer;
          }

          // Now that all planes have been copied, restrcture each of
          // them so they have the size of a tile and use the right
          // pixel-buffer-pointer (in the full framebuffer) as well as
          // the right scanline sizes (ie.: strides).
          FrameBuffer* aTilePlaneFB = tileFB;
          FrameBuffer* aFullPlaneFB = fullFB;
          while( aTilePlaneFB && aFullPlaneFB )
          {
            const int tilePosX = static_cast<int>(
                static_cast<float>( x ) /
                    static_cast<float>( tilingInfo.numTiles ) *
                    static_cast<float>( aFullPlaneFB->width() ) +
                0.5f );
            const int tilePosY = static_cast<int>(
                static_cast<float>( y ) /
                    static_cast<float>( tilingInfo.numTiles ) *
                    static_cast<float>( aFullPlaneFB->height() ) +
                0.5f );

            int tileSizeX = static_cast<int>(
                1.0f / static_cast<float>( tilingInfo.numTiles ) *
                    static_cast<float>( aFullPlaneFB->width() ) +
                0.5f );

            int tileSizeY = static_cast<int>(
                1.0f / static_cast<float>( tilingInfo.numTiles ) *
                    static_cast<float>( aFullPlaneFB->height() ) +
                0.5f );

            // Make sure that the last tiles doe not overshoot the full
            // plane which would cause an access violation.
            // Note that this can happen because of the round up in the
            // previous float to integer conversion.
            // Example: numTiles=6, aFullPlaneFB->height=94605
            tileSizeX = std::min( tileSizeX, aFullPlaneFB->width() - tilePosX );
            tileSizeY =
                std::min( tileSizeY, aFullPlaneFB->height() - tilePosY );

            unsigned char* tilePixels =
                ( aFullPlaneFB->pixels<unsigned char>() +
                  tilePosY * aFullPlaneFB->scanlineSize() +
                  tilePosX * aFullPlaneFB->pixelSize() );

            // This will resize the properties to fit the tile size
            aTilePlaneFB->restructure(
                tileSizeX, tileSizeY, aFullPlaneFB->depth(),
                aFullPlaneFB->numChannels(), aFullPlaneFB->dataType(),
                tilePixels, &aFullPlaneFB->channelNames(),
                aFullPlaneFB->orientation(),
                // The 1st tile becomes owner of the pixel data
                isMasterBuffer /* deleteOnDestruction */,
                aFullPlaneFB->extraScanlines(),
                aFullPlaneFB->scanlinePixelPadding(), NULL,
                false /* do not clear attributes */ );

            // Override the scanline sizes computed during the call to
            // restructure, using the full framebuffer sizes.
            aTilePlaneFB->setScanlineSize( aFullPlaneFB->scanlineSize() );
            aTilePlaneFB->setScanlinePaddedSize(
                aFullPlaneFB->scanlinePaddedSize() );

            if( isMasterBuffer )
            {
              // Flag the master buffer (first tile) so it knows it
              // is responsible for removing the other (proxy buffer)
              // tiles from the cache when it gets evicted.
              aTilePlaneFB->attribute<int>( "MasterBuffer" ) = 1;
            }

            // Proceed to next plane.
            aTilePlaneFB = aTilePlaneFB->nextPlane();
            aFullPlaneFB = aFullPlaneFB->nextPlane();
          }

          tileFB->idstream() << "/tile_x" << x << "_y" << y;

          IPImage* tileIPImage =
              new IPImage( this, IPImage::BlendRenderType, tileFB );

          if( failed )
          {
            tileIPImage->missing = true;
            tileIPImage->missingLocalFrame = request.frame;
            tileIPImage->setErrorState( this, errorString );
          }
          else
          {
            tileIPImage->info = &( mov->info() );

            tileIPImage->shaderExpr =
                Shader::sourceAssemblyShader( tileIPImage );
            tileIPImage->recordResourceUsage();

            // Configure the tile FrameBuffer/IPImage.
            configureAlphaAttrs( tileFB, tileIPImage );
          }

          Matrix M;
          Mat44f S;
          Mat44f T;

          const float frameRatio = static_cast<float>( info.width ) /
                                   static_cast<float>( info.height ) *
                                   info.pixelAspect;

          const float gridSizeX = tilingInfo.numTiles;
          const float gridSizeY = tilingInfo.numTiles;

          const float tileSizeX = frameRatio / gridSizeX;
          const float tileSizeY = 1.0 / gridSizeY;

          const float xPos = static_cast<float>( x );
          const float yPos = static_cast<float>( y );

          // Compute the scaling matrix.
          S.makeScale( Vec3f( 1.0 / gridSizeX, 1.0 / gridSizeY, 1.0 ) );
          M = S * M;

          // Compute the translation matrix.
          float offsetX = ( xPos * tileSizeX ) + ( tileSizeX * 0.5 );
          float offsetY = ( yPos * tileSizeY ) + ( tileSizeY * 0.5 );

          // Offset the positions from bottom-left to center.
          const float centerX = frameRatio / 2.0;
          const float centerY = 0.5;

          offsetX -= centerX;
          offsetY -= centerY;

          // Apply the orientation settings.
          if( mov->info().orientation == FrameBuffer::TOPLEFT ||
              mov->info().orientation == FrameBuffer::TOPRIGHT )
          {
            offsetY = -offsetY;
          }

          T.makeTranslation( Vec3f( offsetX, offsetY, 0.0 ) );
          M = T * M;

          // Apply the global transformation matrix
          tileIPImage->transformMatrix = M * tileIPImage->transformMatrix;

          // Keep the transformation matrix as an attribute so we can
          // re-apply it to any IPImage created when this fb is found
          // in a cache.
          std::vector<float> txMatCoeffs;
          for( int rowIndex = 0; rowIndex < 4; rowIndex++ )
          {
            for( int colIndex = 0; colIndex < 4; colIndex++ )
            {
              txMatCoeffs.push_back(
                  tileIPImage->transformMatrix( rowIndex, colIndex ) );
            }
          }

          TypedFBVectorAttribute<float>* attr =
              new TypedFBVectorAttribute<float>( "TransformMatrix",
                                                 txMatCoeffs );
          tileIPImage->fb->addAttribute( attr );

          ostringstream tileRVSource;
          tileRVSource << name() << "/tile_x" << x << "_y" << y << "."
                       << ( ( context.stereo && context.eye == 1 ) ? 1 : 0 )
                       << "/" << ( va ? va->value() : "0" ) << "/" << lframe;

          tileIPImage->fb->attribute<string>( "RVSource" ) = tileRVSource.str();

          root->appendChild( tileIPImage );
        }
      }

      // Pass the vector of proxy buffers pointers to the master buffer so
      // it can evict every
      if( masterBuffer )
      {
        TypedFBVectorAttribute<FrameBuffer*>* attr =
            new TypedFBVectorAttribute<FrameBuffer*>( "ProxyBuffers",
                                                      proxyBuffers );
        masterBuffer->addAttribute( attr );
      }

      root->blendMode = IPImage::Over;

      root->recordResourceUsage();

      // Set the data pointer to null since data ownership has been passed
      // to the 1st tile. Then delete the full framebuffer (not the data).
      fullFB->relinquishDataAndReset();

      delete fullFB;
      fullFB = nullptr;

      return root;
    }
  }

  IPImageID* FileSourceIPNode::evaluateIdentifier( const Context& context )
  {
    const bool profile =
        ( context.thread & DisplayThread ) && graph()->needsProfilingSamples();
    ImageComponent selection;
    MediaPointer media;
    {
      const QReadLocker readLock( &m_mediaMutex );

      // NOTE: Might be loading still
      if( m_mediaVector.size() == 0 ) return new IPImageID( "No Media" );

      media = getMediaFromContext( selection, context );
    }

    Movie* mov = movieForThread( media.get(), context );
    if( !mov || !media )
    {
      TWK_THROW_EXC_STREAM(
          "ERROR: FileSourceIPNode::evaluateIdentifier no media found." );
    }

    Movie::ReadRequest request( context.frame, context.stereo );
    setupRequest( mov, selection, context, request );

    //
    //  Check coverage
    //

    ImageStructureInfo info = imageStructureInfo( context );

    TilingInfo tilingInfo = getTilingInfo( info.width, info.height );

    //
    //  This may throw -- allow it to escape this function
    //

    Movie::IdentifierVector m;
    mov->identifiersAtFrame( request, m );

    //
    //  Not an error for an audio-only source to have no IDs.  XXX should come
    //  back to this.
    //
    if( !media->hasVideo() ) m.push_back( "no-source-image" );

    if( m.empty() )
      TWK_THROW_EXC_STREAM( "FileSource: movie identifiers eval failed." );

    //
    //  Optionally add in the source name to all ids. See comment in evaluate()
    //  regarding this. Basically, this forces rendered images from different
    //  sources to be unique.
    //
    //  Also add any image attribtues
    //

    ostringstream idstr;

    idstr << m[0] << idFromAttributes();

    if( m_sourceNameInID ) idstr << "/" << name();

    idstr << "." << media->index << "/";

    if( tilingInfo.scale < 1.0f )
    {
      idstr << "*" << tilingInfo.scale;
    }

    // For debugging, use different tiling layouts based on frame number:
    // frame 1 : 1x1
    // frame 2 : 2x2
    // etc.
    if( tilingInfo.numTiles == 1 && DEBUG_ANIMATED_TILING )
    {
      tilingInfo.numTiles = context.frame;
    }

    if( tilingInfo.numTiles == 1 )
    {
      return new IPImageID( idstr.str() );
    }

    IPImageID* rootNode = new IPImageID;
    // This IPImageID has a size that does not fit in a texture, tag it
    // so it is never rendered as an intermediate.
    rootNode->noIntermediate = true;
    IPImageID* prev = 0;

    for( int y = 0; y < tilingInfo.numTiles; y++ )
    {
      for( int x = 0; x < tilingInfo.numTiles; x++ )
      {
        std::string tileIdStr = idstr.str();
        tileIdStr += std::string( "/tile_x" );
        tileIdStr += std::to_string( x );
        tileIdStr += std::string( "_y" );
        tileIdStr += std::to_string( y );
        IPImageID* child = new IPImageID( tileIdStr );
        if( x == 0 && y == 0 )
        {
          rootNode->children = child;
        }
        else
        {
          prev->next = child;
        }
        prev = child;
      }
    }

    return rootNode;
  }

  IPNode::ImageRangeInfo FileSourceIPNode::imageRangeInfo() const
  {
    const QReadLocker readLock( &m_mediaMutex );

    int rangeOffset = m_rangeOffset->front();

    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      auto media = m_mediaVector[i];
      Movie* mov = media->primaryMovie();

      if( hasVideo() && !mov->hasVideo() ) continue;

      int start = mov->info().start + rangeOffset;
      int end = mov->info().end + rangeOffset;

      if( m_rangeStart && m_rangeStart->size() )
      {
        int rs = m_rangeStart->front();
        end = rs - start + end;
        start = rs;
      }

      int cutin = min( end, max( start, m_cutIn->front() ) );
      int cutout = max( start, min( end, m_cutOut->front() ) );

      // Only use undiscovered range in async mode
      const auto isUndiscovered =
          hasProgressiveSourceLoading() && !media->shared->hasValidRange;
      return ImageRangeInfo( start, end, mov->info().inc, fps(), cutin, cutout,
                             isUndiscovered );
    }

    // Only use undiscovered range in async mode
    const auto isUndiscovered = hasProgressiveSourceLoading();
    return ImageRangeInfo( 0, 0, 0, 0.0, 0, 0, isUndiscovered );
  }

  IPNode::ImageStructureInfo FileSourceIPNode::imageStructureInfo(
      const Context& context ) const
  {
    const QReadLocker readLock( &m_mediaMutex );

    int wa = 0;
    int ha = 0;
    int w = 0;
    int h = 0;
    int na = 0;
    int nv = 0;
    float pa = 1.0;
    Mat44f O;

    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Movie* mov = m_mediaVector[i]->primaryMovie();
      bool audio =
          mov->hasAudio() && ( !mov->hasVideo() || !m_noMovieAudio->front() );
      bool video = mov->hasVideo();

      const MovieInfo& info = mov->info();
      const int iw = info.uncropWidth;
      const int ih = info.uncropHeight;
      const float ipa = info.pixelAspect;

      if( audio && !video )
      {
        na++;
        wa = max( wa, iw );
        ha = max( ha, ih );
      }

      if( video )
      {
        nv++;
        w = max( w, iw );
        h = max( h, ih );
        //
        //  pixelaspect may be < or > 1.0 so just adopt any
        //  value that is interesting.
        //
        if( ipa != 0.0 && ipa != 1.0 ) pa = ipa;

        O = orientationMatrix( info.orientation );
      }
    }

    if( !nv && na )
    {
      w = wa;
      h = ha;
    }

    return ImageStructureInfo( w * ( pa > 1.0 ? pa : 1.0 ),
                               h / ( pa < 1.0 ? pa : 1.0 ), pa, O );
  }

  void FileSourceIPNode::audioConfigure( const AudioConfiguration& config )
  {
    const QWriteLocker writeLock(
        &m_mediaMutex );  // Always take the media mutex before the audio one
    LockGuard lockAudio( m_audioMutex );

    m_adevRate = config.rate;
    m_adevLayout = config.layout;
    m_adevSamples = config.samples;

    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Media* media = m_mediaVector[i].get();
      Movie* mov = media->audioMovie();

      if( !mov->hasAudio() || ( mov->hasVideo() && m_noMovieAudio->front() ) )
        continue;

      Movie::AudioConfiguration conf( config.rate, config.layout,
                                      config.samples );
      mov->audioConfigure( conf );

      if( !mov->canConvertAudioRate() )
      {
        double factor = m_adevRate / double( mov->info().audioSampleRate );

        //
        //  The "3" below is a magic number. This may need to
        //  be adjusted if the granular code needs a bigger
        //  window.
        //

        const size_t grainSlop = 3;

        size_t overflow =
            ( timeToSamples( m_grainDuration + m_grainEnvelope * 2.0,
                             m_adevRate ) *
              grainSlop ) /
            m_adevSamples;

        if( overflow <= 1 ) overflow = 2;

        m_adevBufferWindow = m_adevSamples * overflow;

        //
        // To prevent the resampler from returning zero samples at any
        // particular process pass, the blocksize has to be increased at large
        // down sample rates. Otherwise the blocksize is too small for the
        // resampler to complete its process at a single pass and may return
        // zero samples processed to give it time to build up more samples
        // internally.
        //
        size_t blocksize = AUDIO_RESAMPLER_WINDOW_SIZE;
        if( factor < 0.3 )
        {
          // Example 44.1kHz -> 11.025khz/8khz
          blocksize = 2 * AUDIO_RESAMPLER_WINDOW_SIZE;
        }
        int channels = layoutChannels( m_adevLayout ).size();
        media->reset( channels, factor, m_adevBufferWindow, blocksize );
      }
    }
  }

  size_t FileSourceIPNode::audioFillBuffer( const AudioContext& context )
  {
    AudioBuffer& buffer = context.buffer;

    //
    //  There are certain cases where the m_adevRate/m_adevLayout does not equal
    //  the context buffer rate/layout.
    //  This happens when the current media happens to have the
    //  same audio config as the last media that was played.
    //
    //  See audioConfigure in IPGraph.cpp; i.e. the notify flag will be false
    //  and so audioConfigure of FileSourceIPNode is never called and the
    //  current media will have its m_adevRate still at the default value.
    //
    //  So in some cases, this default m_adevRate/m_adevLayout might not equal
    //  the context buffer rate/layout. As such we call audioConfigure
    //  explicitly to ensure the audio config rate and layout matches the
    //  context buffer rate and layout always... (which is the audio config of
    //  the audio renderer in use)
    //

    if( ( buffer.rate() != m_adevRate ) ||
        ( buffer.channels() != TwkAudio::layoutChannels( m_adevLayout ) ) )
    {
      AudioConfiguration config( buffer.rate(),
                                 TwkAudio::channelLayout( buffer.channels() ),
                                 buffer.size() );
      audioConfigure( config );
    }

    const double fpsfactor = context.fps / fps();

    if( fpsfactor == 1.0 )
    {
      //
      //  Normal playback -- no stretching or compressing
      //

      return audioFillBufferInternal( context );
    }

    //
    //  Just an FYI; this code is only ever called when the graph viewnode is
    //  this source and the session/playback fps does not match the source fps.
    //  In all other graph topologies with higher level nodes using FileSource's
    //  as inputs, the context.fps matches the fps().
    //

    const Time rate = buffer.rate();
    const size_t ch = buffer.numChannels();
    const size_t start = buffer.startSample();
    const size_t num = buffer.size();
    const size_t end = start + num;

    const size_t grainSamples = timeToSamples( m_grainDuration, buffer.rate() );
    const size_t grainMarginSamples =
        timeToSamples( m_grainEnvelope, buffer.rate() );
    const size_t grainSyncWidth = grainSamples + grainMarginSamples;

    const size_t sync0 = start / grainSyncWidth;
    const size_t syncN = ( start + num ) / grainSyncWidth;
    const size_t grainSize = grainSyncWidth + grainMarginSamples;

    float* out = buffer.pointer();

    for( size_t i = sync0; i <= syncN; i++ )
    {
      //
      //  Locate the grain we want. t is relative to the output
      //  time.
      //

      Time t = i * samplesToTime( grainSyncWidth, rate ) -
               samplesToTime( grainMarginSamples, rate );
      Time d = samplesToTime( grainSize, rate );

      if( t < 0 ) t = 0;

      AudioBuffer grain( grainSize, buffer.channels(), rate, t * fpsfactor );
      AudioContext rcontext( grain, fps() );

      if( audioFillBufferInternal( rcontext ) == 0 ) break;

      //
      //  render the grain
      //

      const float* in = grain.pointer();
      size_t gpos = timeToSamples( t, rate );

      for( size_t q = 0; q < grainSize; q++, gpos++ )
      {
        if( gpos < start ) continue;
        if( gpos >= end ) break;

        double env = 1.0;

        if( q < grainMarginSamples )
        {
          env = double( q ) / double( grainMarginSamples );
        }
        else if( q > grainSyncWidth )
        {
          env =
              1.0 - double( q - grainSyncWidth ) / double( grainMarginSamples );
        }

        for( size_t c = 0; c < ch; c++ )
        {
          out[( gpos - start ) * ch + c] += env * in[q * ch + c];
        }
      }
    }

    AUDIOBUFFER_CHECK( buffer,
                       "FileSourceIPNode::audioFillBuffer() after grain" )

    return buffer.size();
  }

  size_t FileSourceIPNode::audioFillBufferInternal(
      const AudioContext& context )
  {
    //
    //  There's some fps conversion that happens here: RV always plays
    //  every frame (unless specifically told to do otherwise or it
    //  skips in realtime). What this means, is that given a movie for
    //  example, that has an fps that differs from the playback fps,
    //  we don't do a pull-down on the video, we extend or shorten the
    //  audio instead.
    //
    //  The "playback" fps is passed up the through the audio
    //  context. The start sample and num samples are relative to that
    //  fps. In order to handle fps
    //

    const QReadLocker readLock(
        &m_mediaMutex );  // Always take the media mutex before the audio one

    vector<size_t> audioMedia;
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Media* media = m_mediaVector[i].get();
      Movie* mov = media->audioMovie();

      if( !mov->hasAudio() ) continue;
      if( mov->hasVideo() && m_noMovieAudio->front() ) continue;

      audioMedia.push_back( i );
    }

    context.buffer.zero();  // sanitize the buffer
    size_t targetNumSamples = context.buffer.size();

    //
    //  If we don't have any audio sources or we have an invalid buffer stop now
    //

    if( !audioMedia.size() || !context.buffer.numChannels() ||
        !context.buffer.size() )
    {
      return targetNumSamples;  // buffer full of zeros
    }

    LockGuard lockAudio( m_audioMutex );

    for( vector<size_t>::iterator am = audioMedia.begin();
         am != audioMedia.end(); am++ )
    {
      Media* media = m_mediaVector[*am].get();

      AudioBuffer mediaBuffer( context.buffer.size(), context.buffer.channels(),
                               context.buffer.rate(),
                               offsetStartTime( context ) );

      media->setBackwards( isBackwards() );

      size_t nread = media->audioFillBuffer( mediaBuffer );
      if( nread == 0 ) continue;  // don't mix corrupted reads

      //
      //  Mix into the output stream
      //

      const float volume = taperedClamp( m_volume->front(), 0.0f, 1.0f ) /
                           float( audioMedia.size() );
      const float balance = max( -1.0f, min( m_balance->front(), 1.0f ) );
      const float lvol =
          ( balance > 0.0f ? ( 1.0f - balance ) : 1.0f ) * volume;
      const float rvol =
          ( balance < 0.0f ? ( 1.0f + balance ) : 1.0f ) * volume;
      const bool naudio = ( *am != 0 );

      mixChannels( mediaBuffer, context.buffer, lvol, rvol, naudio );
    }

    return targetNumSamples;
  }

  TwkAudio::Time FileSourceIPNode::offsetStartTime( const AudioContext context )
  {
    const double rate = context.buffer.rate();
    const Time startTime = samplesToTime( context.buffer.startSample(), rate );

    return ( startTime - m_offset->front() ) * ( context.fps / fps() );
  }

  void FileSourceIPNode::flushAllCaches( const FlushContext& context )
  {
    const QReadLocker readLock( &m_mediaMutex );
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      const SharedMoviePointerVector& movies = m_mediaVector[i]->shared->movies;
      for( size_t q = 0; q < movies.size(); q++ ) movies[q]->flush();
    }
  }

  void FileSourceIPNode::invalidateFileSystemInfo()
  {
    const QReadLocker readLock( &m_mediaMutex );
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      const SharedMoviePointerVector& movies = m_mediaVector[i]->shared->movies;
      for( size_t q = 0; q < movies.size(); q++ )
        movies[q]->invalidateFileSystemInfo();
    }
  }

  void FileSourceIPNode::prepareForWrite()
  {
    Vec2i size( 0, 0 );
    {
      // Note: It is possible for m_mediaVector to be empty if the media is
      //       not active for example.
      const QReadLocker readLock( &m_mediaMutex );
      if( !m_mediaVector.empty() )
      {
        size =
            Vec2i( m_mediaVector.front()->primaryMovie()->info().uncropWidth,
                   m_mediaVector.front()->primaryMovie()->info().uncropHeight );
      }
    }

    //
    //  Add information about the movie range, etc, just in case the
    //  movie file becomes inaccessible in when the RV file is read
    //  in. This makes it possible to read a partial RV file. These
    //  are all put in a "proxy" component.
    //

    ImageRangeInfo info = imageRangeInfo();

    //
    //  Use prop info from from mediaMovies so that proxy is not saved in
    //  profiles
    //

    declareProperty<Vec2iProperty>( "proxy.range",
                                    Vec2i( info.start, info.end + 1 ),
                                    m_mediaMovies->info() );
    declareProperty<IntProperty>( "proxy.inc", info.inc,
                                  m_mediaMovies->info() );
    declareProperty<FloatProperty>( "proxy.fps", info.fps,
                                    m_mediaMovies->info() );
    declareProperty<Vec2iProperty>( "proxy.size", size, m_mediaMovies->info() );

    //
    //  Do PATHSWAP mappings.
    //
    //   Note: don't get any security bookmarks (permissions) we need to read
    //   this file when sandboxed, since they should have been added to Source
    //   by higher-level code.
    //

    Bundle* bundle = Bundle::mainBundle();

    for( int i = 0; i < m_mediaMovies->size(); i++ )
    {
      string filePath = ( *m_mediaMovies )[i];
      ( *m_mediaMovies )[i] = Application::mapToVar( filePath );
    }
  }

  void FileSourceIPNode::writeCompleted()
  {
    //
    //  Remove the proxy properties
    //
    if( numMedia() )
    {
      removeProperty( find( "proxy.inc" ) );
      removeProperty( find( "proxy.range" ) );
      removeProperty( find( "proxy.fps" ) );
      removeProperty( find( "proxy.size" ) );
    }

    //
    //  Undo PATHSWAP mappings
    //

    if( StringProperty* sp = m_mediaMovies )
    {
      for( int i = 0; i < sp->size(); i++ )
      {
        ( *sp )[i] = Application::mapFromVar( ( *sp )[i] );
      }
    }

    m_tempComponents.clear();
  }

  void FileSourceIPNode::readCompleted( const std::string& typeName,
                                        unsigned int version )
  {
    {
      HOP_PROF(
          "FileSourceIPNode::readCompleted - SourceIPNode::readCompleted" );
      SourceIPNode::readCompleted( typeName, version );
    }

    //
    //  Map PATHSWAP env vars to paths
    //

    {
      HOP_PROF( "FileSourceIPNode::readCompleted - Application::mapFromVar" );
      if( StringProperty* sp = m_mediaMovies )
      {
        for( int i = 0; i < sp->size(); i++ )
        {
          ( *sp )[i] = Application::mapFromVar( ( *sp )[i] );
        }
      }
    }

    if( IntProperty* p = property<IntProperty>( "group.rangeStart" ) )
    {
      m_rangeStart = p;
    }

    if( isMediaActive() )
    {
      HOP_PROF( "FileSourceIPNode::readCompleted - reloadMediaFromFiles" );
      reloadMediaFromFiles();
    }
  }

  std::string FileSourceIPNode::filename() const
  {
    const StringProperty* sp = m_mediaMovies;
    ostringstream str;

    for( int i = 0; i < sp->size(); i++ )
    {
      if( i ) str << "|";
      str << ( *sp )[i];
    }

    return str.str();
  }

  void FileSourceIPNode::clearMedia()
  {
    const QWriteLocker writeLock( &m_mediaMutex );
    m_mediaVector.clear();
    setHasAudio( false );
    setHasVideo( false );
    m_mediaMovies->resize( 0 );
    m_mediaViewNames->resize( 0 );
    m_eyeViews->resize( 0 );
  }

  FileSourceIPNode::Movie* FileSourceIPNode::movieByMediaIndex( size_t index )
  {
    const QReadLocker readLock( &m_mediaMutex );
    if( index < m_mediaVector.size() )
    {
      return m_mediaVector[index]->primaryMovie();
    }
    else
    {
      return 0;
    }
  }

  const FileSourceIPNode::Movie* FileSourceIPNode::movieByMediaIndex(
      size_t index ) const
  {
    const QReadLocker readLock( &m_mediaMutex );
    if( index < m_mediaVector.size() )
    {
      return m_mediaVector[index]->primaryMovie();
    }
    else
    {
      return 0;
    }
  }

  FileSourceIPNode::Movie* FileSourceIPNode::movieByMediaName(
      const string& name )
  {
    const QReadLocker readLock( &m_mediaMutex );
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Movie* mov = m_mediaVector[i]->primaryMovie();

      if( MovieReader* reader = dynamic_cast<MovieReader*>( mov ) )
      {
        if( reader->filename() == name ) return reader;
      }
    }

    return 0;
  }

  const FileSourceIPNode::Movie* FileSourceIPNode::movieByMediaName(
      const string& name ) const
  {
    const QReadLocker readLock( &m_mediaMutex );
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Movie* mov = m_mediaVector[i]->primaryMovie();

      if( MovieReader* reader = dynamic_cast<MovieReader*>( mov ) )
      {
        if( reader->filename() == name ) return reader;
      }
    }

    return 0;
  }

  float FileSourceIPNode::fps() const
  {
    if( m_fps->size() && m_fps->front() != 0.0 )
    {
      return m_fps->front();
    }

    const QReadLocker readLock( &m_mediaMutex );

    float nfps = 0.0, afps = 0.0;

    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Movie* mov = m_mediaVector[i]->primaryMovie();

      //
      //  Some dpx files in particular have garbage value in fps header
      //  fields, which can be 0 or a very small number.  We assume here that
      //  any fps < 0.5 is a mistake.
      //

      if( mov->info().fps > 0.5 )
      {
        if( mov->hasVideo() )
          nfps = mov->info().fps;
        else
          afps = mov->info().fps;
      }
    }

    if( nfps == 0.0 ) nfps = afps;
    if( nfps != 0.0 ) return nfps;

    //
    //  If we get to this point, we have no clue from the media about the fps,
    //  and the user did not specify it.  So we're going to use the defaultFPS.
    //  This is a _preference_ so different users loading the same sequence can
    //  get different FPSs, furthermore rvio does not read prefs, so it always
    //  assumes fps=24 at this point.  If we save this session and reload
    //  (possibly in rvio, or with different prefs) we want to get the same
    //  fps.  So if there was at least one piece of media, we retain the
    //  assumed fps in the property.
    //

    if( !m_mediaVector.empty() ) m_fps->front() = defaultFPS;

    return defaultFPS;
  }

  bool FileSourceIPNode::isBackwards() const
  {
    //
    // We need to determine the direction of audio play.
    // NB: We assume that there is no intervening node that might
    //     cause the lastAudioConfiguration().backwards to be incorrect.
    //

    return graph()->lastAudioConfiguration().backwards;
  }

  void FileSourceIPNode::newPropertyCreated( const Property* p )
  {
    if( IntProperty* rs = property<IntProperty>( "group", "rangeStart" ) )
    {
      if( rs == p ) m_rangeStart = rs;
    }

    IPNode::newPropertyCreated( p );
  }

  void FileSourceIPNode::propertyDeleted( const std::string& propName )
  {
    if( propName == name() + ".group.rangeStart" )
    {
      m_rangeStart = 0;
      propagateRangeChange();
    }

    IPNode::propertyDeleted( propName );
  }

  void FileSourceIPNode::updateHasAudioStatus()
  {
    const QWriteLocker writeLock( &m_mediaMutex );

    bool hasAudio = false;
    for( int i = 0; i < m_mediaVector.size(); ++i )
    {
      SharedMoviePointerVector smpv = m_mediaVector[i]->shared->movies;

      for( int j = 0; j < smpv.size(); ++j )
      {
        if( smpv[j]->hasAudio() &&
            ( !smpv[j]->hasVideo() || !m_noMovieAudio->front() ) )
        {
          hasAudio = true;
          break;
        }
      }
    }
    setHasAudio( evIgnoreAudio.getValue() ? false : hasAudio );

    if( graph() ) graph()->updateHasAudioStatus( this );
  }

  void FileSourceIPNode::propertyChanged( const Property* p )
  {
    //
    //  If a prop changed that affects the cached state of all frames from
    //  source, flush all those IDs from cache.
    //

    if( p == m_noMovieAudio )
    {
      updateHasAudioStatus();
    }

    if( p == m_readAllChannels || p == m_imageComponent || p == m_eyeViews )
    {
      if( group() ) group()->flushIDsOfGroup();
    }
    else if( p == m_offset || p == m_volume || p == m_balance ||
             p == m_crossover || p == m_noMovieAudio )
    {
      graph()->flushAudioCache();
    }
    else if( p == m_rangeOffset || p == m_rangeStart || p == m_cutOut ||
             p == m_cutIn )
    {
      propagateRangeChange();
    }
    else if( p == m_fps )
    {
      graph()->flushAudioCache();
      propagateRangeChange();
    }
    else if( p == m_mediaMovies || p == m_mediaActive )
    {
      if( isMediaActive() )
      {
        reloadMediaFromFiles();
      }
    }

    SourceIPNode::propertyChanged( p );
  }

  void FileSourceIPNode::propertyWillChange( const Property* p )
  {
    IPNode::propertyWillChange( p );
  }

  size_t FileSourceIPNode::numMedia() const
  {
    const QReadLocker readLock( &m_mediaMutex );
    return m_mediaVector.size();
  }

  size_t FileSourceIPNode::mediaIndex( const std::string& name ) const
  {
    const QReadLocker readLock( &m_mediaMutex );
    for( size_t i = 0; i < m_mediaVector.size(); i++ )
    {
      Movie* mov = m_mediaVector[i]->primaryMovie();

      if( MovieReader* reader = dynamic_cast<MovieReader*>( mov ) )
      {
        if( reader->filename() == name ) return i;
      }
    }

    return size_t( -1 );
  }

  const SourceIPNode::MovieInfo& FileSourceIPNode::mediaMovieInfo(
      size_t index ) const
  {
    if( const Movie* mov = movieByMediaIndex( index ) )
    {
      return mov->info();
    }
    else
    {
      TWK_THROW_STREAM( LayerOutOfBoundsExc,
                        "Index " << index << " is out-of-bounds" );
    }
  }

  const string& FileSourceIPNode::mediaName( size_t index ) const
  {
    if( const MovieReader* mov =
            dynamic_cast<const MovieReader*>( movieByMediaIndex( index ) ) )
    {
      return mov->filename();
    }
    else
    {
      TWK_THROW_STREAM( LayerOutOfBoundsExc,
                        "Index " << index << " is out-of-bounds" );
    }
  }

  void FileSourceIPNode::setReadAllChannels( bool v )
  {
    if( m_readAllChannels )
    {
      m_readAllChannels->resize( 1 );
      m_readAllChannels->front() = ( v ) ? 1 : 0;
    }
  }

  void FileSourceIPNode::cancelJobs()
  {
    m_jobsCancelling++;

    cancelAllDispatchJobs();

    //
    // Remove the work item if any
    //

    int workItemID = m_workItemID.exchange( 0 );
    if( workItemID )
    {
      // Remove the job if not already started.
      graph()->removeWorkItem( workItemID );

      // Wait for its completion if already started.
      graph()->waitWorkItem( workItemID );
    }

    undispatchAllDispatchJobs();
    resetLoadingID();

    m_jobsCancelling--;
  }

  void FileSourceIPNode::reloadMediaFromFiles()
  {
    HOP_PROF_FUNC();

    cancelJobs();

    //
    //  Make a copy of the media values
    //

    StringVector media = m_mediaMovies->valueContainer();

    //
    //  Clear it out
    //

    clearMedia();

    //
    //  Restore what we're about to read
    //

    m_mediaMovies->valueContainer() = media;

    //
    // First assume that the media files are all available (active)
    // Note that if any of the media file associated with this FileSourceIPNode
    // is unreachable, then setMediaActive(false) will be called.
    // For reference : FileSourceIPNode::openMovieTask()
    //
    m_mediaActive->front() = 1;

    //
    //  Sync media files
    //

    Bundle* bundle = Bundle::mainBundle();
    SharedMediaPointerVector sharedMedias( m_mediaMovies->size() );
    for( size_t i = 0; i < m_mediaMovies->size(); i++ )
    {
      std::string filename( ( *m_mediaMovies )[i] );

      // Note: This progressive source loading mechanism does not work well
      // with procedural movies : frame#1 is always black.
      // As a work-around we are not using asynchronous loading with movieproc
      if( hasProgressiveSourceLoading() &&
          TwkUtil::extension( filename ) != "movieproc" )
      {
        // In async mode, start by loading a proxy movie.
        ostringstream errorString;
        errorString << "Loading " << filename << "...";

        Movie* mov = openProxyMovie( errorString.str(),
                                     2.0,  // wait 2 sec to display error
                                     filename, defaultFPS );

        SharedMediaPointer sharedMedia(
            newSharedMedia( mov, find( "proxy.range" ) ) );
        sharedMedias[i] = sharedMedia;
        addMedia( sharedMedia );
      }
      else
      {
        filename = lookupFilenameInMediaLibrary( filename );

        openMovieTask( filename, SharedMediaPointer() );
      }
    }

    if( hasProgressiveSourceLoading() )
    {
      if( !media.empty() )
      {
        // Lookup the filenames in the media library, save associated HTTP header and cookies
        // if any, and return the actual filename to be used for opening the movie in case of 
        // redirection.
        // Note: This needs to be done in the main thread, NOT in the worker thread
        // because the media library is python based and thus should only be executed 
        // in the main thread.
        for( size_t i = 0; i < media.size(); i++ )
        {
          media[i] = lookupFilenameInMediaLibrary( media[i] );
        }

        // Then add a work item to load the actual media movies.
        m_workItemID = graph()->addWorkItem(
            [this, sharedMedias, media]()
            {
              if( !media.empty() )
              {
                for( size_t i = 0; i < media.size(); i++ )
                {
                  openMovieTask( media[i], sharedMedias[i] );
                }
              }
            } );

        // Add it to the set of media currently loading
        setLoadingID( m_workItemID );
      }
    }
  }

  void FileSourceIPNode::openMovieTask(
      const string& filename, const SharedMediaPointer& proxySharedMedia )
  {
    HOP_PROF_FUNC();

    debuggingLoadDelay();

    //
    //  NOTE: addMedia() will cause a propagateRangeChange() down the
    //  graph using this thread (which is usually some worker thread
    //  in IPGraph).
    //

    Movie* mov = 0;
    bool failed = false;
    ostringstream errMsg;

    try
    {
      mov = openMovie( filename );
    }
    catch( std::exception& exc )
    {
      failed = true;
      errMsg << "Open of '" << filename << "' failed: " << exc.what();

      cerr << "ERROR: " << errMsg.str() << endl;
    }
    catch( ... )
    {
      failed = true;
    }

    if( !mov || failed )
    {
      if( errMsg.tellp() == std::streampos( 0 ) )
      {
        errMsg << "Could not locate \"" << filename
               << "\". Relocate source to fix.";
      }

      mov = openProxyMovie( errMsg.str(), 0.0, filename, defaultFPS );

      ostringstream str;
      str << name() << ";;" << filename << ";;" << mediaRepName();
      TwkApp::GenericStringEvent event( "source-media-unavailable", graph(), str.str() );

      // The following instructions can only be executed on the main thread.
      // Dispatch to main thread only when using async loading.
      if (!m_workItemID)
      {
        // We are not using async loading: we can safely execute the following instructions
        setMediaActive( false );
        graph()->sendEvent( event );
      }
      else
      {
        // We are using async loading: Dispatch to main thread
        addDispatchJob( Application::instance()->dispatchToMainThread(
            [this, event]( Application::DispatchID dispatchID )
            {
              {
                LockGuard dispatchGuard( m_dispatchIDCancelRequestedMutex );

                bool isCanceled =
                    ( m_dispatchIDCancelRequestedSet.count( dispatchID ) >= 1 );
                if( isCanceled )
                {
                  m_dispatchIDCancelRequestedSet.erase( dispatchID );
                  return;
                }
              }

              setMediaActive( false );
              graph()->sendEvent( event );

              {
                LockGuard dispatchGuard( m_dispatchIDCancelRequestedMutex );
                bool isCanceled =
                    ( m_dispatchIDCancelRequestedSet.count( dispatchID ) >= 1 );
                if( isCanceled )
                {
                  m_dispatchIDCancelRequestedSet.erase( dispatchID );
                  return;
                }
              }

              removeDispatchJob( dispatchID );
            } ) );
      }
    }

    SharedMediaPointer sharedMedia(
        newSharedMedia( mov, true /*hasValidRange*/ ) );
    addMedia( sharedMedia, proxySharedMedia );
  }

  string FileSourceIPNode::cacheHash( const string& filename,
                                      const string& prefix )
  {
    string hashString;

    const bool filepathIsURL = TwkUtil::pathIsURL( filename );
    if( !filepathIsURL )
    {
      if(TwkUtil::fileExists(filename.c_str()))
      {
        try
        {
          boost::filesystem::path p( UNICODE_STR( filename ) );
          const auto size = boost::filesystem::file_size( p );

          //
          //  should this function be hashing the first 256 bytes or so of
          //  the file to be safe?
          //

          ostringstream name;
          boost::hash<string> string_hash;
          name << filename << "::" << size;
          ostringstream fullHash;
          fullHash << prefix << hex << string_hash( name.str() ) << dec;
          hashString = fullHash.str();
        }
        catch( ... )
        {
          // nothing
        }
      }
    }

    return hashString;
  }

  string FileSourceIPNode::lookupFilenameInMediaLibrary( const string& filename )
  {
    string file(filename);

    if( TwkMediaLibrary::isLibraryURL( file ) )
    {
      //
      //  If this file looks like a library URL try to convert it
      //  into a local file URL and then a path
      //

      file = TwkMediaLibrary::libraryURLtoMediaURL( file );
      file.erase( 0, 7 );  // assuming "file://" not good
      return file;
    }

    if( TwkMediaLibrary::isLibraryMediaURL( file ) )
    {
      //
      //  URL is a media URL tracked by one of the libraries. Find its
      //  media node and find out if its streaming.
      //

      TwkMediaLibrary::NodeVector nodes =
          TwkMediaLibrary::libraryNodesAssociatedWithURL( file );
      for( int n = 0; n < nodes.size(); n++ )
      {
        const TwkMediaLibrary::Node* node = nodes[n];
        std::string nodeName = node->name();

        if( const TwkMediaLibrary::MediaAPI* api =
                TwkMediaLibrary::api_cast<TwkMediaLibrary::MediaAPI>( node ) )
        {
          if( api->isStreaming() )
          {
            TwkMediaLibrary::HTTPCookieVector cookies = api->httpCookies();
            if( !cookies.empty() )
            {
              ostringstream cookieStm;

              for( int c = 0; c < cookies.size(); c++ )
              {
                if( c > 0 ) cookieStm << "\n";
                cookieStm << cookies[c].name << "=" << cookies[c].value;

                if( !cookies[c].path.empty() )
                  cookieStm << "; path=" << cookies[c].path;
                if( !cookies[c].domain.empty() )
                  cookieStm << "; domain=" << cookies[c].domain;
              }

              if (evDebugCookies.getValue()) std::cout << "Cookies:\n" << cookieStm.str() << std::endl;

              m_inparams.push_back( StringPair( "cookies", cookieStm.str() ) );
            }

            TwkMediaLibrary::HTTPHeaderVector headers = api->httpHeaders();
            if( !headers.empty() )
            {
              ostringstream headersStm;

              for( size_t h = 0, size = headers.size(); h < size; ++h )
              {
                if( h > 0 ) headersStm << "\r\n";
                headersStm << headers[h].name << ": " << headers[h].value;
              }

              if (evDebugCookies.getValue()) std::cout << "Headers:\n" << headersStm.str() << std::endl;

              m_inparams.push_back( StringPair( "headers", headersStm.str() ) );
            }
          }

          if( api->isRedirecting() )
          {
            std::string redirection = api->httpRedirection();
            std::cout << "INFO: " << nodeName << " is redirecting " << file
                      << " to " << redirection << std::endl;
            file = redirection;
          }
        }
      }
    }

    return file;
  }

  bool FileSourceIPNode::findCachedMediaMetaData( const string& filename,
                                                  PropertyContainer* pc )
  {
    //
    //  Check this node first -- we may find it here if a session is being
    //  read
    //

    Bundle* bundle = Bundle::mainBundle();
    string cacheItemString = cacheHash( filename, "movpd_" );

    if( cacheItemString == "" ) return false;

    if( Component* c = component( cacheItemString ) )
    {
      const Components& innercomps = c->components();

      for( size_t q = 0; q < innercomps.size(); q++ )
      {
        pc->add( innercomps[q]->copy() );
        return true;
      }
    }

    //
    //  Look up a cach file for this in the MediaMetadata cache
    //

    if( bundle->hasCacheItem( "MediaMetadata", cacheItemString ) )
    {
      Bundle::Path p =
          bundle->cacheItemPath( "MediaMetadata", cacheItemString );
      TwkContainer::GTOReader reader( true );
      reader.read( p.c_str() );
      const TwkContainer::GTOReader::Containers& containers =
          reader.containersRead();
      assert( containers.size() == 1 );

      for( size_t i = 0; i < containers.size(); i++ )
      {
        TwkContainer::PropertyContainer* c = containers[i];

        if( c->name() == cacheItemString )
        {
          pc->copy( c );
          delete c;
          return true;
        }
      }
    }

    return false;
  }

  FileSourceIPNode::Movie* FileSourceIPNode::openMovie( const string& filename )
  {
    // Ensure file exists and is accessible by the current user before
    // continuing
    //
    if( !TwkUtil::pathIsURL( filename ) && TwkUtil::extension( filename ) != "movieproc" )
    {
      const auto firstFileInSeq =
          TwkUtil::firstFileInPattern( filename.c_str() );
      const char* pathToTest =
          firstFileInSeq.empty() ? filename.c_str() : firstFileInSeq.c_str();

      if( !TwkUtil::isReadable( pathToTest ) )
      {
        TWK_THROW_EXC_STREAM( strerror( errno ) );
      }
    }

    MovieInfo info;
    PropertyContainer* pc = new PropertyContainer();
    findCachedMediaMetaData( filename, pc );
    info.privateData = pc;

    Movie::ReadRequest request;
    std::copy( m_inparams.begin(), m_inparams.end(),
               back_inserter( request.parameters ) );
    MovieReader* reader =
        TwkMovie::GenericIO::openMovieReader( filename, info, request );

    if( reader && reader->needsScan() )
    {
      reader->scan();

      string cacheItemString = cacheHash( filename, "movpd_" );

      Bundle* bundle = Bundle::mainBundle();
      string cachePath =
          bundle->cacheItemPath( "MediaMetadata", cacheItemString );
      boost::filesystem::create_directories(
          boost::filesystem::path( UNICODE_STR( cachePath ) ).parent_path() );

      TwkContainer::GTOWriter writer;
      TwkContainer::GTOWriter::ObjectVector objs( 1 );
      objs[0] = TwkContainer::GTOWriter::Object( pc, cacheItemString,
                                                 "MovieIOPrivatedata", 1 );
      writer.write( cachePath.c_str(), objs, Gto::Writer::CompressedGTO );

      bundle->addCacheItem( "MediaMetaData", cacheItemString );
    }

    return reader;
  }

  FileSourceIPNode::Movie* FileSourceIPNode::openProxyMovie(
      const string& errorString, double minBeforeTime, const string filename,
      double defaultFPS )
  {
    Vec2i range = propertyValue<Vec2iProperty>( "proxy.range", Vec2i( 1, 20 ) );
    int inc = propertyValue<IntProperty>( "proxy.inc", 1 );
    Vec2i size =
        propertyValue<Vec2iProperty>( "proxy.size", Vec2i( 1280, 720 ) );
    float fps = propertyValue<FloatProperty>( "proxy.fps", defaultFPS );

    ostringstream proxy;
    proxy << definition()->stringValue( "defaults.missingMovieProc", "black" )
          << ",start=" << range[0] << ",end=" << range[1] - 1 << ",inc=" << inc
          << ",width=" << size[0] << ",height=" << size[1] << ",fps=" << fps
          << ",filename=" << TwkUtil::id64Encode( filename )
          << ",attr:MissingFile=1";
    if( !errorString.empty() )
    {
      proxy << ",errorString=" << TwkUtil::id64Encode( errorString )
            << ",minBeforeTime="
            << minBeforeTime;  // wait 2 seconds before to display this message
    }
    proxy << ".movieproc";

    Movie::ReadRequest request( range[0] );
    MovieInfo info;
    MovieReader* reader =
        TwkMovie::GenericIO::openMovieReader( proxy.str(), info, request );

    return reader;
  }

  void FileSourceIPNode::setLoadingID( IPGraph::WorkItemID newLoadingID )
  {
    auto prevLoadingID = m_loadingID.exchange( newLoadingID );

    if( newLoadingID == prevLoadingID ) return;

    if( newLoadingID ) graph()->mediaLoadingBegin( newLoadingID );

    if( prevLoadingID ) graph()->mediaLoadingEnd( prevLoadingID );
  }

  void FileSourceIPNode::addDispatchJob( IPGraph::WorkItemID id )
  {
    LockGuard lock( m_dispatchIDListMutex );

    m_dispatchIDList.push_back( id );
  }

  void FileSourceIPNode::removeDispatchJob( IPGraph::WorkItemID id )
  {
    LockGuard lock( m_dispatchIDListMutex );

    m_dispatchIDList.remove( id );

    if( m_dispatchIDList.empty() )
      // Remove it from the set of media currently loading
      resetLoadingID();
  }

  void FileSourceIPNode::cancelAllDispatchJobs()
  {
    LockGuard lock( m_dispatchIDListMutex );

    for( auto id : m_dispatchIDList )
    {
      LockGuard dispatchGuard( m_dispatchIDCancelRequestedMutex );
      m_dispatchIDCancelRequestedSet.insert( id );
    }
  }

  void FileSourceIPNode::undispatchAllDispatchJobs()
  {
    LockGuard lock( m_dispatchIDListMutex );

    for( auto id : m_dispatchIDList )
      Application::instance()->undispatchToMainThread( id, 5.0 /*secs*/ );

    m_dispatchIDList.clear();
  }
}  // namespace IPCore
