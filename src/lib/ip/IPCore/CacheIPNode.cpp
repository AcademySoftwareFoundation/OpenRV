//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/CacheIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderCommon.h>
#include <iostream>
#include <IPCore/IPGraph.h>
#include <TwkFB/Cache.h>
#include <TwkMath/Frustum.h>
#include <TwkUtil/sgcHop.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

#if 0
#define DB_GENERAL 0x01
#define DB_ICON 0x02
#define DB_ADD 0x04
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_ICON))
//  #define DB_LEVEL        DB_ALL
//  #define DB_LEVEL        DB_GENERAL
#define DB_LEVEL DB_ADD

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << "CacheIPNode: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << "CacheIPNode: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

    CacheIPNode::CacheIPNode(const std::string& name, const NodeDefinition* def,
                             const IPNode* snode, IPGraph* g,
                             GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_active(false)
        , m_sourceNode(snode)
    {
        init();
    }

    CacheIPNode::CacheIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_active(false)
        , m_sourceNode(0)
    {
        init();
    }

    CacheIPNode::~CacheIPNode() { pthread_mutex_destroy(&m_mutex); }

    void CacheIPNode::init()
    {
        setMaxInputs(1);
        pthread_mutex_init(&m_mutex, 0);

        m_performDownSample =
            declareProperty<IntProperty>("render.downSampling", 1);
    }

    //
    //  Converts an IPImageID tree into a IPImage tree by looking up the
    //  ids in the cache and creating IPImages to hold them. Should be
    //  handed to transform_ip<>()
    //

    struct IPImageTreeFromIDTree
    {
        IPImageTreeFromIDTree(const IPNode::Context& c, const CacheIPNode* n)
            : context(c)
            , missed(true)
            , node(n)
        {
        }

        const IPNode::Context& context;
        bool missed;
        const CacheIPNode* node;

        IPImage* operator()(IPImageID* id)
        {
            IPImage* img = 0;

            try
            {
                DB("IPImageTreeFromIDTree: calling checkOut " << id->id);
                TwkFB::FrameBuffer* fb = context.cache.checkOut(id->id);
                const IPNode* snode = node->sourceNode();
                img = fb ? (new IPImage(snode, IPImage::BlendRenderType, fb))
                         : (new IPImage(snode));

                // Transfer the noIntermediate flag from the IPImageID to the
                // associated IPImage.
                if (id->noIntermediate)
                {
                    img->noIntermediate = true;
                }

                if (fb)
                {
                    if (const TwkFB::TypedFBVectorAttribute<float>*
                            transformMatrixAtt = dynamic_cast<
                                const TwkFB::TypedFBVectorAttribute<float>*>(
                                fb->findAttribute("TransformMatrix")))
                    {
                        if (transformMatrixAtt
                            && transformMatrixAtt->value().size() == 16)
                        {
                            std::vector<float> txMatCoeffs;
                            for (int rowIndex = 0; rowIndex < 4; rowIndex++)
                            {
                                for (int colIndex = 0; colIndex < 4; colIndex++)
                                {
                                    img->transformMatrix(rowIndex, colIndex) =
                                        transformMatrixAtt
                                            ->value()[rowIndex * 4 + colIndex];
                                }
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                DB("IPImageTreeFromIDTree: checkOut " << id->id << " threw");
                img = new IPImage(node);
            }

            if (!img->fb)
            {
                DB("IPImageTreeFromIDTree: checkOut " << id->id << " failed");
            }
            else
            {
                // We only need one hit to consider it a success.
                missed = false;
            }

            return img;
        }
    };

    //
    //  Assign source shaders. This has to be the last pass in case we
    //  reused a cached fb (it can get deleted out from under the
    //  expression)
    //

    struct AssignSourceExpression
    {
        void operator()(IPImage* img)
        {
            // assert(!img->shaderExpr);
            delete img->shaderExpr;
            img->shaderExpr = 0;

            if (img->fb)
            {
                img->shaderExpr = Shader::sourceAssemblyShader(img);
                img->resourceUsage =
                    img->shaderExpr->computeResourceUsageRecursive();
            }
        }
    };

    //
    //  Checks in every fb in an IPImage tree. Should be called by foreach_ip
    //

    struct CheckInImageFBs
    {
        CheckInImageFBs(const IPNode::Context& c)
            : context(c)
        {
        }

        const IPNode::Context& context;

        void operator()(IPImage* img)
        {
            if (img->fb)
            {
                DB("evaluate:    after miss, checkIn of "
                   << img->fb << " " << img->fb->identifier());
                context.cache.Cache::checkIn(img->fb);
            }
            else
                DB("evaluate:    after miss, skipping null fb");
        }
    };

    //
    //  Complicated: this thing tries to check out the fb in the image. If
    //  it can, it swaps the cached one for the evaluated one. Otherwise,
    //

    struct UseCacheImageIfExists
    {
        UseCacheImageIfExists(const IPNode::Context& c, bool m,
                              IPNode::ThreadType t, IPImage* r)
            : context(c)
            , missing(m)
            , thread(t)
            , root(r)
        {
        }

        bool missing;
        const IPNode::Context& context;
        IPImage* root;
        IPNode::ThreadType thread;

        void operator()(IPImage* img)
        {
            TwkFB::FrameBuffer* fb = img->fb;
            img->missing = missing;
            img->missingLocalFrame = context.frame;

            // We support visiting IPImage nodes with no FrameBuffer. We just
            // skip them.
            if (!fb)
                return;

            TWK_CACHE_LOCK(context.cache,
                           "thread=" << thread << ", img=" << img);

            TwkFB::FrameBuffer* cfb = 0;

            try
            {
                DB("UseCacheImageIfExists: calling checkOut "
                   << fb->identifier());
                cfb = context.cache.checkOut(fb->identifier());
            }
            catch (std::exception& exc)
            {
                // TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
                // err << exc.what() << endl;
                // abort();
                // throw;
                DB("UseCacheImageIfExists: calling checkOut "
                   << fb->identifier());
                DB("UseCacheImageIfExists: cache checkOut of "
                   << fb->identifier() << " threw: " << exc.what());

                cfb = 0;
            }

            if (cfb)
            {
                //
                //  This one was in the cache. So use the cached version (it
                //  might be locked) and delete the one that was just evaluated.
                //
                //  But note that the frame buffer _is_ required for this frame
                //  (it just came back from evaluate), so make sure it's
                //  associated with this frame in the cache.
                //
                //  If context.cacheNode, we are evaluating for an explice
                //  per-node cache of this fb, but since we found it in the
                //  cache this fb must already have been cached for some other
                //  reason.  In this case, we need to _not_ associated it with
                //  any frame, and need to instead reference it in the per-node
                //  cache.
                //

                if (context.cacheNode)
                {
                    context.cache.add(cfb, context.baseFrame, false,
                                      context.cacheNode);
                }
                else
                {
                    context.cache.referenceFrame(context.baseFrame, cfb);
                }

                TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);

                DB("UseCacheImageIfExists: found fb "
                   << cfb << " " << cfb->identifier() << " in cache, so");
                DB("UseCacheImageIfExists:     deleting evaluated fb "
                   << fb << " " << fb->identifier());
                delete fb;
                fb = cfb;
                img->fb = cfb;
            }
            else
            {
                DB("UseCacheImageIfExists: calling add, thread "
                   << context.thread << ":" << context.threadNum << " id "
                   << fb->identifier());

                //
                //  If we're going to possibly shove the fb into the cache, make
                //  sure we are not at least already overflowing.
                //
                if (context.cache.used() > context.cache.capacity())
                    context.cache.emergencyFree();

                //
                //  XXX  We are forcing ("true" in next line) this fb into the
                //  cache even if it is already full.  We find that avfoundation
                //  reader can cause giant hiccup if we ask for out of order
                //  frames during playback, which is what happens here if we
                //  discard a frame when the cache fills during playback (we ask
                //  for the same frame twice).  Further investigation needed.
                //
                bool addSucceeded = context.cache.add(fb, context.baseFrame,
                                                      true, context.cacheNode);
                DB("UseCacheImageIfExists: add succeeded: " << addSucceeded);

                if (!addSucceeded && thread == IPNode::CacheEvalThread)
                {
                    //
                    //  Only the cache thread gets here:
                    //
                    //  Cache filled, delete results and throw. Some of
                    //  the results may have been cached; those img->fb's
                    //  will not be deleted and the next time around those
                    //  sources will be found in the cache.
                    //
                    DB("UseCacheImageIfExists: evaluateIdentifier failed");

                    try
                    {
                        DBL(DB_ADD, "UseCacheImageIfExists: add failed");
                        DBL(DB_ADD, "UseCacheImageIfExists:     "
                                    "checkInAndDelete after add failure, root "
                                        << root);
                        context.cache.checkInAndDelete(root);
                        DBL(DB_ADD, "UseCacheImageIfExists:     "
                                    "checkInAndDelete after add failure, done");
                    }
                    catch (...)
                    {
                        //
                    }

                    TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);

                    DB("UseCacheImageIfExists: throwing CacheFullExc, thread "
                       << context.threadNum);
                    throw CacheFullExc();
                }

                try
                {
                    if (fb->inCache())
                    {
                        context.cache.checkOut(fb);
                    }
                    DB("UseCacheImageIfExists: fb now in cache");
                }
                catch (...)
                {
                    TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
                    abort();
                    throw;
                }

                TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
            }
        }
    };

    //
    //  Checks in every fb in an IPImage tree. Should be called by foreach_ip
    //

    struct AddToCacheAtFrame
    {
        AddToCacheAtFrame(const IPNode::Context& c, bool m)
            : context(c)
            , missing(m)
        {
        }

        const IPNode::Context& context;
        bool missing;

        void operator()(IPImage* img)
        {
            if (missing)
            {
                img->missing = true;
                img->missingLocalFrame = context.frame;
            }

            // NOTE: already checked out
            if (img->fb)
            {
                context.cache.add(img->fb, context.baseFrame, false,
                                  context.cacheNode);
            }
        }
    };

#define PROFILE_SAMPLE(P, X)                                                 \
    if (P)                                                                   \
    {                                                                        \
        IPGraph::EvalProfilingRecord& r = graph()->currentProfilingSample(); \
        r.X = graph()->profilingElapsedTime();                               \
    }

    IPImage* CacheIPNode::evaluate(const Context& context)
    {
        DB(name() << " evaluate() frame " << context.frame
                  << " ****************************************************");

        ThreadType thread = context.thread;
        IPNode* inNode = inputs().front();

        //  XXX  Thread type is now unused
        if (thread == DisplayPassThroughThread)
        {
            return IPNode::evaluate(context);
        }

        //
        //  Overview: for each input image see if we can retrieve from the
        //  cache (regardless of what thread this is). If we can great, if
        //  not do something thread appropriate.
        //

        IPImageID* idTree = 0;
        bool missing = false;
        bool missed = false;
        bool profile =
            (thread & DisplayThread) && graph()->needsProfilingSamples();

        PROFILE_SAMPLE(profile, evalIDStart);

        try
        {
            //
            //  Find the cache ids that will be required when
            //  evaluated. If possible use these later to build an IPImage
            //  tree instead of doing real eval.
            //
            //  This will throw if the identifier is "out-of-range" or
            //  missing. So we then tell it give us an appropriate proxy
            //  for this frame.
            //

            idTree = evaluateIdentifier(context);

            // cout << "Acc = " << idTree->id << endl << M0 << endl;
        }
        catch (...)
        {
            missing = true;
            Context mcontext = context;
            mcontext.missing = true;

            try
            {
                idTree = evaluateIdentifier(mcontext);
            }
            catch (...)
            {
                //  ever happen?
                DB("evaluate: evaluateIdentifier failed");
                missed = true;
            }
        }

        PROFILE_SAMPLE(profile, evalIDEnd);

        //
        //  See if there's a cached version of each fb in the tree
        //

        PROFILE_SAMPLE(profile, cacheQueryStart);

        TWK_CACHE_LOCK(context.cache, "thread=" << thread);

        IPImageTreeFromIDTree F(context, this);

        DB("evaluate: calling transform_ip with callable "
           "IPImageTreeFromIDTree ");
        IPImage* root =
            transform_ip<IPImage, IPImageID, IPImageTreeFromIDTree>(idTree, F);
        missed = F.missed || missed;

        if (missed)
        {
            DB("evaluate: cache miss frame " << context.frame << " thread "
                                             << context.thread << ":"
                                             << context.threadNum);

            //
            //  Check them back in. We'll check them out one at a time
            //  later.
            //

            try
            {
                DB("evaluate:     checking in evaluated fbs");
                CheckInImageFBs cifbs(context);
                foreach_ip(root, cifbs);
            }
            catch (...)
            {
                // shouldn't get here
                assert(true);
            }

            delete root;
            root = 0;

            TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
            delete idTree; // clean up previously used identifiers

            PROFILE_SAMPLE(profile, cacheQueryEnd);

            if (thread == DisplayNoEvalThread)
            {
                //
                //  If this is DisplayNoEvalThread, the cache may be
                //  inconsistant. Perhaps new media was added to a source
                //  and is not represented in the cache or a change in the
                //  configuration of the graph occured or some other
                //  circumstance that causes the cache to no longer be
                //  complete. Let the IPGraph figure out what it wants to
                //  do about it.
                //

                DB("evaluate: throwing CacheMissExc");
                throw CacheMissExc();
            }

            //
            //  Evaluate the images.
            //

            PROFILE_SAMPLE(profile, cacheEvalStart);

            if (!missing)
            {
                root = inNode->evaluate(context);
            }
            else
            {
                Context mcontext = context;
                mcontext.missing = true;
                root = inNode->evaluate(mcontext);
            }

            DB("evaluate: calling UseCacheImageIfExists");
            UseCacheImageIfExists Fcache(context, missing, thread, root);
            foreach_ip(root, Fcache);

            PROFILE_SAMPLE(profile, cacheEvalEnd);
        }
        else
        {
            //
            //  All of the fbs were in the cache. Append them to the image
            //  list. Also, add them back into the cache for this
            //  frame. This makes sure all ids for the current frame are
            //  recorded.
            //
            AddToCacheAtFrame A(context, missing);
            foreach_ip(root, A);
            TWK_CACHE_UNLOCK(context.cache, "thread=" << thread);
            delete idTree; // clean up previously used identifiers

            PROFILE_SAMPLE(profile, cacheQueryEnd);
        }

        AssignSourceExpression Fassign;
        foreach_ip(root, Fassign);

        static bool use_std_filtering = getenv("RV_USE_STD_FILTERING");
        if (m_performDownSample->front() && !use_std_filtering
            && root->shaderExpr)
        {
            HOP_PROF("CacheIPNode::evaluate() - root->shaderExpr = "
                     "Shader::newDerivativeDownSample");
            root->shaderExpr =
                Shader::newDerivativeDownSample(root->shaderExpr);
        }

        return root;
    }

    struct UsePerNodeCacheContents
    {
        UsePerNodeCacheContents(const IPNode::Context& c, const CacheIPNode* n)
            : context(c)
            , cacheNode(n)
        {
        }

        const IPNode::Context& context;
        bool missed;
        const CacheIPNode* cacheNode;

        void operator()(IPImageID* id)
        {
            if (context.cacheNode)
            {
                TwkFB::FrameBuffer* fb =
                    context.cache.perNodeCacheContents(context.cacheNode);
                if (fb)
                    id->id = fb->identifier();
            }
        }
    };

    IPImageID* CacheIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* id = IPNode::evaluateIdentifier(context);

        //
        //  If this is a child of a TextureOutputGroupIPNode _and_ this is not a
        //  caching thread, we allow the evaluation to use possibly out of date
        //  contents of the per-node cache.
        //

        if (context.cacheNode && context.thread != CacheEvalThread)
        {
            UsePerNodeCacheContents F(context, this);
            foreach_ip(id, F);
        }

        return id;
    }

    void CacheIPNode::flushAllCaches(const FlushContext& context)
    {
        try
        {
            IPImageID* idtree = evaluateIdentifier(context);

            try
            {
                IPNode::FlushFBs F(context);
                foreach_ip(idtree, F);
                delete idtree;
            }
            catch (...)
            {
                delete idtree;
                throw;
            }
        }
        catch (std::exception& exc)
        {
            //
            //  If there's no ID for this frame, that should mean it was never
            //  cached in the first place, so it's not an error to be unable to
            //  flush it from the cache now.
            //
        }
    }

} // namespace IPCore
