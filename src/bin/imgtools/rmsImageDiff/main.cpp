//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "../../utf8Main.h"

#ifdef PLATFORM_DARWIN
#include <DarwinBundle/DarwinBundle.h>
#else
#include <QTBundle/QTBundle.h>
#endif
#include <IOproxy/IOproxy.h>
#include <MovieProxy/MovieProxy.h>
#include <MovieProcedural/MovieProcedural.h>
#include <MovieFB/MovieFB.h>
#include <TwkMovie/MovieIO.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/IO.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Math.h>
#include <TwkMath/Color.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Daemon.h>
#include <TwkUtil/PathConform.h>
#include <TwkMath/MatrixAlgo2.h>
#include <TwkMath/Iostream.h>
#include <algorithm>
#include <arg.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <stdlib.h>
#include <math.h>

using namespace std;
using namespace TwkUtil;
using namespace TwkFB;
using namespace TwkMath;

FrameBuffer* convertBoth(FrameBuffer* inFB, FrameBuffer::DataType t)
{
    if (inFB->isPlanar())
        return copyConvert(inFB, t);
    return copyConvertPlane(inFB, t);
}

template <typename T> double sumChannels(const T& v)
{
    double sum = 0.0;
    for (size_t n = 0; n < v.dimension(); ++n)
    {
        sum += (double)v[n];
    }

    return sum;
}

template <typename Tunsigned, typename Tdouble, typename Tfloat>
int computeRMS(const FrameBuffer* a, const FrameBuffer* b, bool doFloat,
               bool findMax, bool doCompare, double dmax)
{

    Tdouble sum(0.0);

    double maxDiff = 0.0;
    const double ushort_max = (double)(std::numeric_limits<ushort>::max());

    int maxX = -1;
    int maxY = -1;

    bool noMatch = false;

    if (doFloat)
    {
        Tfloat maxA;
        Tfloat maxB;

        for (int y = 0; y < a->height(); y++)
        {
            for (int x = 0; x < a->width(); x++)
            {
                Tfloat ca = a->scanline<Tfloat>(y)[x];
                Tfloat cb = b->scanline<Tfloat>(y)[x];

                Tdouble cd = Tdouble(ca) - Tdouble(cb);
                Tdouble cd_norm = (Tdouble(ca) - Tdouble(cb)) / ushort_max;
                Tdouble sq = cd_norm * cd_norm;
                sum += sq;
                if (findMax)
                {
                    double sumSq = sumChannels<Tdouble>(sum);
                    if (sumSq > maxDiff)
                    {
                        maxDiff = sumSq;
                        maxX = x;
                        maxY = y;
                        maxA = ca;
                        maxB = cb;
                    }
                }
                else if (doCompare)
                {
                    for (int c = 0; c < a->numChannels(); ++c)
                    {
                        if (fabs((double)cd[c]) > dmax)
                        {
                            cout << "Pixel difference at (" << x << ", " << y
                                 << ") channel[" << c
                                 << "] = " << fabs((double)cd[c]) << " found."
                                 << endl;
                            noMatch = true;
                            break;
                        }
                    }
                }
                if (noMatch)
                    break;
            }
            if (noMatch)
                break;
        }

        if (doCompare)
        {
            if (noMatch)
            {
                cout << "Images are NOT matched." << endl;
                return 1;
            }
            else
            {
                cout << "Images are matched." << endl;
                return 0;
            }
        }

        sum /= Tdouble(a->height() * a->width());

        for (size_t n = 0; n < sum.dimension(); ++n)
        {
            cout << "" << sqrt(sum[n]);
        }
        cout << endl;

        if (findMax && maxX > -1)
        {
            cout << "max diff at (" << maxX << ", " << maxY << "):  " << maxA
                 << " vs. " << maxB << endl;
        }
    }
    else
    {
        Tunsigned maxA;
        Tunsigned maxB;

        for (int y = 0; y < a->height(); y++)
        {
            for (int x = 0; x < a->width(); x++)
            {
                Tunsigned ca = a->scanline<Tunsigned>(y)[x];
                Tunsigned cb = b->scanline<Tunsigned>(y)[x];

                Tdouble cd = Tdouble(ca) - Tdouble(cb);
                Tdouble cd_norm = (Tdouble(ca) - Tdouble(cb)) / ushort_max;
                Tdouble sq = cd_norm * cd_norm;
                sum += sq;
                if (findMax)
                {
                    double sumSq = sumChannels<Tdouble>(sum);
                    if (sumSq > maxDiff)
                    {
                        maxDiff = sumSq;
                        maxX = x;
                        maxY = y;
                        maxA = ca;
                        maxB = cb;
                    }
                }
                else if (doCompare)
                {
                    for (int c = 0; c < a->numChannels(); ++c)
                    {
                        if (fabs((double)cd[c]) > dmax)
                        {
                            cout << "Pixel difference at (" << x << ", " << y
                                 << ") channel[" << c
                                 << "] = " << fabs((double)cd[c]) << " found."
                                 << endl;
                            noMatch = true;
                            break;
                        }
                    }
                }
                if (noMatch)
                    break;
            }
            if (noMatch)
                break;
        }

        if (doCompare)
        {
            if (noMatch)
            {
                cout << "Images are NOT matched." << endl;
                return 1;
            }
            else
            {
                cout << "Images are matched." << endl;
                return 0;
            }
        }

        sum /= Tdouble(a->height() * a->width());
        for (size_t n = 0; n < sum.dimension(); ++n)
        {
            cout << "" << sqrt(sum[n]);
        }
        cout << endl;

        if (findMax && maxX > -1)
        {
            cout << "max diff at (" << maxX << ", " << maxY << "):  " << maxA
                 << " vs. " << maxB << endl;
        }
    }

    return 0;
}

int utf8Main(int argc, char* argv[])
{
#ifdef PLATFORM_DARWIN
    TwkApp::DarwinBundle bundle("RV", MAJOR_VERSION, MINOR_VERSION,
                                REVISION_NUMBER);
#else
    QCoreApplication qapp(argc, argv);
    TwkApp::QTBundle bundle("rv", MAJOR_VERSION, MINOR_VERSION,
                            REVISION_NUMBER);
    (void)bundle.top();
#endif

    char* input1 = 0;
    char* input2 = 0;

    bool doFloat = false;
    bool findMax = false;
    bool doCompare = false;
    double dmax = 0.0;

    //
    //  Parse cmd line args
    //

    string usage =
        string("usage: ") + argv[0]
        + " [-f] [-m] [-cmp] [-dmax <value>] <image1> <image2>\n"
        + "    -f: operate on FLOAT pixels\n"
        + "    -m: print location and values of pixel with max difference\n"
        + "    -cmp: perform image comparison (returns exit 1 if not matched)"
        + "    -dmax: Maximum allow channel error during comparison "
          "(default=0.0)";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-f") == 0)
            doFloat = true;
        else if (strcmp(argv[i], "-m") == 0)
            findMax = true;
        else if (strcmp(argv[i], "-cmp") == 0)
            doCompare = true;
        else if (strcmp(argv[i], "-dmax") == 0 && (i + 1 < argc))
            dmax = atof(argv[++i]);
        else if (!input1)
            input1 = argv[i];
        else if (!input2)
            input2 = argv[i];
    }
    if (argc < 3 || argc > 8 || !input1 || !input2)
    {
        cerr << usage;
        exit(-1);
    }

    string in1 = pathConform(input1);
    string in2 = pathConform(input2);

    TwkFB::GenericIO::init();    // Initialize TwkFB::GenericIO plugins statics
    TwkMovie::GenericIO::init(); // Initialize TwkMovie::GenericIO plugins
                                 // statics

    try
    {
        TwkFB::loadProxyPlugins("TWK_FB_PLUGIN_PATH");
        TwkMovie::loadProxyPlugins("TWK_MOVIE_PLUGIN_PATH");
    }
    catch (...)
    {
        cerr << "WARNING: a problem occured while loading plugins." << endl;
        cerr << "         some plugins may not have been loaded." << endl;
    }

    TwkMovie::GenericIO::addPlugin(new TwkMovie::MovieFBIO());
    TwkMovie::GenericIO::addPlugin(new TwkMovie::MovieProceduralIO());

    //
    //  Add the statically linked in types
    //

    GenericIO::FrameBufferVector aImages, bImages;
    FrameBufferIO::ReadRequest request;

    try
    {
        cout << "INFO: reading " << in1 << endl;
        GenericIO::readImages(aImages, in1, request);
        cout << "INFO: reading " << in2 << endl;
        GenericIO::readImages(bImages, in2, request);
    }
    catch (exception& exc)
    {
        cerr << "ERROR: while reading images: " << exc.what() << endl;
        exit(-1);
    }

    FrameBuffer* ai = aImages.front();
    FrameBuffer* bi = bImages.front();

    //    cout << "a -> "; ai->outputInfo(cout); cout << " planar: " <<
    //    ai->isPlanar() << endl; cout << "b -> "; bi->outputInfo(cout); cout <<
    //    " planar: " << bi->isPlanar() << endl;
    if (ai->isPlanar())
    {
        FrameBuffer* temp = mergePlanes(ai);
        ai = temp;
    }
    if (bi->isPlanar())
    {
        FrameBuffer* temp = mergePlanes(bi);
        bi = temp;
    }

    FrameBuffer* a =
        convertBoth(ai, (doFloat) ? FrameBuffer::FLOAT : FrameBuffer::USHORT);
    FrameBuffer* b =
        convertBoth(bi, (doFloat) ? FrameBuffer::FLOAT : FrameBuffer::USHORT);

    if (a->width() != b->width() || a->height() != b->height())
    {
        cerr << "ERROR: incompatible images" << endl;
        cerr << "       sizes do not match" << endl;
        exit(-1);
    }

    if (a->numChannels() != b->numChannels())
    {
        cerr << "ERROR: incompatible images" << endl;
        cerr << "       channel size does not match" << endl;
        exit(-1);
    }

    if (a->numChannels() > 4 || b->numChannels() > 4)
    {
        cerr << "ERROR: only works on images with 1-4 channels." << endl;
        exit(-1);
    }

    if (a->orientation() != b->orientation())
    {
        flip(b);
    }

    //    cout << "a -> "; a->outputInfo(cout); cout << " planar: " <<
    //    a->isPlanar() << endl; cout << "b -> "; b->outputInfo(cout); cout << "
    //    planar: " << b->isPlanar() << endl;

    //    if (a->dataType() != b->dataType())
    //    {
    //        FrameBuffer::DataType t;

    //        if (a->dataType() < b->dataType())
    //        {
    //            t = a->dataType();
    //        }
    //        else
    //        {
    //            t = b->dataType();
    //        }

    //        if (a->dataType() != t) a = convertBoth(a, t);
    //        if (b->dataType() != t) b = convertBoth(b, t);
    //    }
    //

    int status = 0;

    if (a->numChannels() == 1)
    {
        status = computeRMS<Col1us, Col1d, Col1f>(a, b, doFloat, findMax,
                                                  doCompare, dmax);
    }
    else if (a->numChannels() == 2)
    {
        status = computeRMS<Col2us, Col2d, Col2f>(a, b, doFloat, findMax,
                                                  doCompare, dmax);
    }
    else if (a->numChannels() == 3)
    {
        status = computeRMS<Col3us, Col3d, Col3f>(a, b, doFloat, findMax,
                                                  doCompare, dmax);
    }
    else if (a->numChannels() == 4)
    {
        status = computeRMS<Col4us, Col4d, Col4f>(a, b, doFloat, findMax,
                                                  doCompare, dmax);
    }

    TwkMovie::GenericIO::shutdown(); // Shutdown TwkMovie::GenericIO plugins
    TwkFB::GenericIO::shutdown();    // Shutdown TwkFB::GenericIO plugins

    return 0;
}
