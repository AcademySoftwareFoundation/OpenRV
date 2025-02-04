//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <TwkMovie/NullWriter.h>
#include <TwkUtil/Timer.h>
#include <limits>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkUtil;

    NullWriter::NullWriter()
        : MovieWriter()
    {
    }

    NullWriter::~NullWriter() {}

    bool NullWriter::write(Movie* inMovie, const string& filename,
                           WriteRequest& request)
    {
        Frames frames;
        vector<float> audioBuffer;
        bool verbose = request.verbose;
        bool copyfb = request.codec == "copy";

        if (copyfb)
        {
            cout << "INFO: copy test enabled" << endl;
        }

        int fs = inMovie->info().start;
        int fe = inMovie->info().end;
        int inc = inMovie->info().inc;
        float fps = inMovie->info().fps;

        if (request.timeRangeOverride)
        {
            fps = request.fps;
        }

        if (request.timeRangeOverride)
        {
            frames.resize(request.frames.size());
            copy(request.frames.begin(), request.frames.end(), frames.begin());
        }
        else
        {
            for (int i = fs; i <= fe; i += inc)
            {
                frames.push_back(i);
            }
        }

        Timer timer;
        timer.start();

        double last = 0;
        double lastf = 0;
        double minf = numeric_limits<double>::max();
        double maxf = numeric_limits<double>::min();

        for (int i = 0; i < frames.size(); i++)
        {
            int f = frames[i];

            if (f < fs || f > fe)
                continue;

            FrameBufferVector fbs;
            inMovie->imagesAtFrame(f, fbs);

            if (copyfb)
            {
                for (unsigned int i = 0; i < fbs.size(); i++)
                {
                    FrameBuffer* nfb = fbs[i]->copy();
                    nfb->ownData();
                    delete nfb;
                }
            }

            for (unsigned int q = 0; q < fbs.size(); q++)
            {
                delete fbs[q];
            }

            double t = timer.elapsed();
            double tf = t - lastf;
            lastf = t;

            maxf = max(maxf, tf);
            minf = min(minf, tf);

            if (verbose && ((t - last) > 1.0 || f == frames.back()))
            {
                cout << "INFO: stats:"
                     << " " << double(i) / t << " fps, "
                     << "[" << (1.0 / minf) << ", " << (1.0 / maxf) << "] "
                     << " ("
                     << int(float(i) / float(frames.size() - 1) * 10000.0)
                            / float(100.0)
                     << "% done)" << endl;

                last = t;
            }
        }

        return true;
    }

} // namespace TwkMovie

#ifdef _MSC_VER
#undef NOMINMAX
#endif
