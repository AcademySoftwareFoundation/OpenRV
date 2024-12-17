//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MovieMistika/MovieMistika.h>
#include <IOcin/Read8Bit.h>
#include <IOcin/Read10Bit.h>
#include <IOcin/Read16Bit.h>
#include <TwkFB/IO.h>
#include <TwkMath/Function.h>
#include <TwkMovie/Exception.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMath/Color.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <string>
#include <errno.h>
#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#ifndef uint32_t
#define uint32_t __int32
#endif
#endif

namespace TwkMovie
{

    using namespace std;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace TwkMovie;

    static int timecodeToFrames(std::string tc, float fps)
    {
        int hours = atoi(tc.substr(0, 2).c_str());
        int minutes = atoi(tc.substr(3, 2).c_str());
        int seconds = atoi(tc.substr(6, 2).c_str());
        int frames = atoi(tc.substr(9, 2).c_str());
        return (3600 * hours + 60 * minutes + seconds) * fps + frames;
    };

#define DM_PACKING_RGB (1000)
#define DM_PACKING_RGBA (1002)
#define DM_PACKING_YUV422 (1008)
#define DM_PACKING_YUV422X2 (1100)
#define DM_PACKING_RGB10 (8001)
#define DM_PACKING_YUV422_10 (8100)
#define DM_PACKING_YUV422_10X2 (8101)
#define DM_PACKING_RGBA_16 (8200)
#define DM_PACKING_RGBA_EXR (8300)

#define MAGIC (395726)
#define BLOCK_SIZE (4096)

    class MistikaFileHeader
    {
    public:
        uint32_t magicNumber; // must be 395726
        uint32_t version;     // only version 3 currently used
        uint32_t numFrames;
        uint32_t blockSize; // should be 4096
        uint32_t truncMode; // irrelevant for reading
        uint32_t sizeX;
        uint32_t sizeY;
        uint32_t
            packing; // should be one of the above defined DM_PACKING values
        double rate;
        uint32_t interlacing; // 0: progressive, 1: interlaced.
        char timeCode[128];   // null terminated string containing timecode,
                            // preferebly as "frame of day", like 90000, but can
                            // be 01:20:30:11 style too
        char tapeName[128]; // null terminated tape name string

        uint32_t getMagic() { return MAGIC; }

        MistikaFileHeader()
        {
            magicNumber = MAGIC;
            version = 3;
            numFrames = 0;
            truncMode = 1;
            blockSize = BLOCK_SIZE;
            sizeX = 0;
            sizeY = 0;
            packing = 0;
            rate = 0.0;
            interlacing = 0;
        }

        uint32_t GetBytesPerFrame()
        {
            int size = 0;
            switch (packing)
            {
            case DM_PACKING_RGB:
                size = sizeY * sizeX * 3;
                break;
            case DM_PACKING_RGBA:
                size = sizeY * sizeX * 4;
                break;
            case DM_PACKING_YUV422:
                size = sizeY * sizeX * 2;
                break;
            case DM_PACKING_YUV422X2:
                size = sizeY * sizeX * 4;
                break;
            case DM_PACKING_RGB10:
                size = sizeY * sizeX * 4;
                break;
            case DM_PACKING_YUV422_10:
                size = (sizeY * sizeX * 5) / 2;
                break;
            case DM_PACKING_YUV422_10X2:
                size = sizeY * sizeX * 5;
                break;
            case DM_PACKING_RGBA_16:
                size = sizeY * sizeX * 8;
                break;
            case DM_PACKING_RGBA_EXR:
                size = sizeY * sizeX * 8;
                break;
            default:
                size = 0;
                break;
            }
            // pad to a multiple of blockSize
            size = ((size + blockSize - 1) / blockSize) * blockSize;
            return size;
        }

        void swapEndian(void* ptr, uint32_t size)
        {
            char* p = (char*)ptr;
            for (int i = 0; i < size / 2; i++)
            {
                char aux = p[i];
                p[i] = p[size - 1 - i];
                p[size - 1 - i] = aux;
            }
        }

        void swapEndianAll()
        {
            swapEndian(&version, sizeof(uint32_t));
            swapEndian(&magicNumber, sizeof(uint32_t));
            swapEndian(&numFrames, sizeof(uint32_t));
            swapEndian(&blockSize, sizeof(uint32_t));
            swapEndian(&truncMode, sizeof(uint32_t));
            swapEndian(&sizeX, sizeof(uint32_t));
            swapEndian(&sizeY, sizeof(uint32_t));
            swapEndian(&packing, sizeof(uint32_t));
            swapEndian(&rate, sizeof(double));
            swapEndian(&interlacing, sizeof(uint32_t));
        }

        void print()
        {
            printf("---js file header dump---\n");
            printf("Magic Number:\t%d\n", magicNumber);
            printf("Version:\t%d\n", version);
            printf("Frames:\t\t%d\n", numFrames);
            printf("Block size:\t%d\n", blockSize);
            printf("Trunc mode:\t%d\n", truncMode);
            printf("Size X:\t\t%d\n", sizeX);
            printf("Size Y:\t\t%d\n", sizeY);
            printf("Packing:\t%d\n", packing);
            printf("Rate:\t\t%f\n", rate);
            printf("Interlacing:\t%d\n", interlacing);
            printf("TC:\t\t[%s]\n", timeCode);
            printf("Tape:\t\t[%s]\n", tapeName);
        }

        std::string lookupPacking(int packing)
        {
            std::string name;
            switch (packing)
            {
            case 1000:
                name = "RGB";
                break;
            case 1002:
                name = "RGBA";
                break;
            case 1008:
                name = "YUV422";
                break;
            case 1100:
                name = "YUV422X2";
                break;
            case 8001:
                name = "RGB10";
                break;
            case 8100:
                name = "YUV422_10";
                break;
            case 8101:
                name = "YUV422_10X2";
                break;
            case 8200:
                name = "RGBA_16";
                break;
            case 8300:
                name = "RGBA_EXR";
                break;
            default:
                name = "Uknonwn";
                break;
            }
            return name;
        }
    };

    MovieMistika::Format MovieMistika::pixelFormat = MovieMistika::RGB8_PLANAR;

    MovieMistika::MovieMistika()
        : MovieReader()
        , m_swap(false)
        , m_header(new MistikaFileHeader())
    {
        m_threadSafe = true;
    }

    MovieMistika::~MovieMistika()
    {
        delete m_header;
        m_header = 0;
    }

    MovieReader* MovieMistika::clone() const
    {
        MovieMistika* mov = new MovieMistika();
        if (filename() != "")
            mov->open(filename());
        return mov;
    }

    void MovieMistika::open(const string& filename, const MovieInfo& info,
                            const Movie::ReadRequest& request)
    {
        m_filename = filename;
        FILE* file = TwkUtil::fopen(m_filename.c_str(), "r");
        if (!file)
        {
            TWK_THROW_STREAM(IOException,
                             "Can not open Mistika file " << filename);
        }

        if (fread(m_header, sizeof(MistikaFileHeader), 1, file) != 1)
        {
            fclose(file);
            TWK_THROW_STREAM(IOException,
                             "Can not read Mistika header: " << filename);
        }
        fclose(file);

        //
        // See if we have a valid Mistika file and the correct endian-ness
        //

        if (m_header->getMagic() != m_header->magicNumber)
        {
            m_header->swapEndianAll();
            m_swap = true;
        }
        if (m_header->getMagic() != m_header->magicNumber)
        {
            TWK_THROW_STREAM(IOException, "Not a Mistika movie: " << filename);
        }

        //
        // Hard coded test for the one format we currently support
        //

        if (m_header->lookupPacking(m_header->packing) != "RGB10")
        {
            TWK_THROW_STREAM(IOException,
                             "Unsupported Mistika packing format: "
                                 << m_header->lookupPacking(m_header->packing)
                                 << " (" << m_header->packing << ")");
        }

        if (m_header->numFrames <= 0)
        {
            TWK_THROW_STREAM(IOException,
                             "Found no Mistika frames in: " << m_filename);
        }

        //
        // If there is a timeCode in the Mistika header take it into account
        //

        string tc = string(m_header->timeCode);
        int start = (tc.find(":") != string::npos)
                        ? timecodeToFrames(tc, m_header->rate)
                        : atoi(m_header->timeCode);

        m_info.start = start;
        m_info.end = start + m_header->numFrames - 1;
        m_info.inc = 1;
        m_info.fps = m_header->rate;
        m_info.width = m_header->sizeX;
        m_info.height = m_header->sizeY;
        m_info.uncropWidth = m_info.width;
        m_info.uncropHeight = m_info.height;
        m_info.uncropX = 0;
        m_info.uncropY = 0;
        m_info.audio = false;
        m_info.pixelAspect = 1;
        m_info.video = true;
        m_info.orientation = FrameBuffer::TOPLEFT;
        m_info.numChannels =
            3; // XXX this needs adjusting when we support more formats.

        switch (m_header->packing)
        {
        case DM_PACKING_RGB10:
            m_info.proxy.setTransferFunction(ColorSpace::CineonLog());
            break;
        default:
            break;
        }
    }

    void MovieMistika::imagesAtFrame(const ReadRequest& request,
                                     FrameBufferVector& fbs)
    {
        int frame = request.frame;
        int frameOffset = frame - m_info.start;
        fbs.resize(1);
        if (!fbs.front())
            fbs.front() = new FrameBuffer();
        FrameBuffer& fb = *fbs.front();

        //
        // Get the first frame. The header oocupies the first frame's worth of
        // space so make sure to skip past that.
        //

        uint32_t frameSize = m_header->GetBytesPerFrame();
        uint32_t firstFrameOffset = frameSize;

        unsigned char* frameB =
            TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, frameSize);
        if (!frameB)
        {
            TWK_THROW_EXC_STREAM("Error reading Mistika file "
                                 << filename() << ", out of memory");
        }
        //
        // But if we decide to use this mem in-place, we won't free it here.
        //
        bool doDelete = true;

#ifdef PLATFORM_WINDOWS
        HANDLE file;
        DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;

        file =
            CreateFileW(UNICODE_C_STR(m_filename.c_str()), GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, flags, NULL);

        if (file == INVALID_HANDLE_VALUE)
        {
            TWK_THROW_EXC_STREAM("CreateFile: cannot open "
                                 << m_filename
                                 << ", error: " << GetLastError());
        }

        LARGE_INTEGER inout;
        inout.QuadPart = 0;

        LARGE_INTEGER offset;
        offset.QuadPart = __int64(frameSize) * __int64(frameOffset)
                          + __int64(firstFrameOffset);

        if (0 == SetFilePointerEx(file, offset, &inout, FILE_BEGIN))
        {
            CloseHandle(file);
            TWK_THROW_EXC_STREAM("SetFilePointer: failed reading "
                                 << m_filename << ", error " << GetLastError());
        }

        DWORD nread = 0;

        if (!ReadFile(file, frameB, frameSize, &nread, NULL))
        {
            CloseHandle(file);
            TWK_THROW_EXC_STREAM("ReadFile: failed reading "
                                 << m_filename << ", error " << GetLastError());
        }
        CloseHandle(file);
#else
        FILE* file = TwkUtil::fopen(m_filename.c_str(), "r");
        if (!file)
        {
            TWK_THROW_STREAM(IOException,
                             "Can not open Mistika file " << filename());
        }
        if (0
            != fseeko(file,
                      off_t(frameSize) * off_t(frameOffset)
                          + off_t(firstFrameOffset),
                      SEEK_SET))
        {
            fclose(file);
            TWK_THROW_STREAM(IOException,
                             "Could not seek to Mistika frame: " << frame);
        }
        if (fread(frameB, frameSize, 1, file) != 1)
        {
            fclose(file);
            TWK_THROW_STREAM(IOException,
                             "Could not read Mistika frame number: " << frame);
        }
        fclose(file);
#endif

        //
        // Pick the correct Mistika packing -> TwkFB pixel format
        //

        switch (m_header->packing)
        {
            //        case DM_PACKING_RGBA:
            //        case DM_PACKING_YUV422: //Works
            //            fb.restructure(m_header->sizeX, m_header->sizeY, 0, 1,
            //            FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8, (unsigned
            //            char*)frameB, NULL, FrameBuffer::TOPLEFT); break;
            ////        case DM_PACKING_YUV422X2:
            //        case DM_PACKING_RGB:
            //            switch (pixelFormat)
            //            {
            //                case RGB8:
            //                    Read8Bit::readRGB8(m_filename, (unsigned
            //                    char*)frameB, fb, m_header->sizeX,
            //                    m_header->sizeY, 0, false, false, 0); break;
            //                case RGBA8:
            //                    Read8Bit::readRGBA8(m_filename, (unsigned
            //                    char*)frameB, fb, m_header->sizeX,
            //                    m_header->sizeY, 0, false, false, 0); break;
            //                case RGB8_PLANAR:
            //                default:
            //                    FrameBuffer::StringVector chans(3);
            //                    chans[0] = "B"; chans[1] = "G"; chans[2] =
            //                    "R"; fb.restructure(m_header->sizeX,
            //                    m_header->sizeY, 0, 3, FrameBuffer::UCHAR,
            //                    (unsigned char*)frameB, &chans,
            //                    FrameBuffer::BOTTOMLEFT);
            ////                    Read8Bit::readRGB8_PLANAR(m_filename,
            ///(unsigned char*)frameB, fb, m_header->sizeX, m_header->sizeY, 0,
            /// m_swap);
            //                    break;
            //                case RGB10_A2:
            //                case RGB16:
            //                case RGBA16:
            //                case RGB16_PLANAR:
            //            }
            //            break;
            ////        case DM_PACKING_YUV422_10:
            ////        case DM_PACKING_YUV422_10X2:
        case DM_PACKING_RGB10:
            switch (pixelFormat)
            {
            case RGB8:
                Read10Bit::readRGB8(m_filename, (unsigned char*)frameB, fb,
                                    m_header->sizeX, m_header->sizeY, 0,
                                    m_swap);
                break;
            case RGBA8:
                Read10Bit::readRGBA8(m_filename, (unsigned char*)frameB, fb,
                                     m_header->sizeX, m_header->sizeY, 0, false,
                                     m_swap);
                break;
            case RGB8_PLANAR:
            default:
                Read10Bit::readRGB8_PLANAR(m_filename, (unsigned char*)frameB,
                                           fb, m_header->sizeX, m_header->sizeY,
                                           0, m_swap);
                break;
            case RGB10_A2:
                Read10Bit::readRGB10_A2(m_filename, (unsigned char*)frameB, fb,
                                        m_header->sizeX, m_header->sizeY, 0,
                                        m_swap, true, (unsigned char*)frameB);
                doDelete = false;
                break;
            case RGB16:
                Read10Bit::readRGB16(m_filename, (unsigned char*)frameB, fb,
                                     m_header->sizeX, m_header->sizeY, 0,
                                     m_swap);
                break;
            case RGBA16:
                Read10Bit::readRGBA16(m_filename, (unsigned char*)frameB, fb,
                                      m_header->sizeX, m_header->sizeY, 0,
                                      false, m_swap);
                break;
            case RGB16_PLANAR:
                Read10Bit::readRGB16_PLANAR(m_filename, (unsigned char*)frameB,
                                            fb, m_header->sizeX,
                                            m_header->sizeY, 0, m_swap);
                break;
            }
            break;
            ////        case DM_PACKING_RGBA_EXR:
            //        case DM_PACKING_RGBA_16:
            //            switch (pixelFormat)
            //            {
            ////                case RGB8:
            ////                case RGBA8:
            ////                case RGB8_PLANAR:
            ////                case RGB10_A2:
            ////                case RGB16:
            ////                    Read16Bit::readRGB16(m_filename, (unsigned
            /// char*)frameB, fb, m_header->sizeX, m_header->sizeY, 0, m_swap,
            /// false, 0); /                    break; /                case
            /// RGBA16: /                    Read16Bit::readRGBA16(m_filename,
            ///(unsigned char*)frameB, fb, m_header->sizeX, m_header->sizeY, 0,
            /// true, m_swap, false, 0); /                    break;
            //                default:
            //                case RGB16_PLANAR: // Works
            //                    fb.restructure(m_header->sizeX,
            //                    m_header->sizeY, 0, 4, FrameBuffer::USHORT,
            //                    (unsigned char*)frameB, NULL,
            //                    FrameBuffer::BOTTOMLEFT);
            ////                    Read16Bit::readRGB16_PLANAR(m_filename,
            ///(unsigned char*)frameB, fb, m_header->sizeX, m_header->sizeY, 0,
            /// m_swap);
            //                    break;
            //            }
            //            break;
        default:
            TWK_THROW_STREAM(IOException, "Unsupported Mistika packing format: "
                                              << m_header->packing);
            break;
        }

        if (doDelete)
            TWK_DEALLOCATE(frameB);

        fb.setIdentifier("");
        identifier(frame, fb.idstream());
        fb.addAttribute(new IntAttribute("Mistika/Version", m_header->version));
        fb.addAttribute(
            new IntAttribute("Mistika/Frames", m_header->numFrames));
        fb.addAttribute(new StringAttribute(
            "Mistika/Packing", m_header->lookupPacking(m_header->packing)));
        fb.addAttribute(new FloatAttribute("Mistika/Rate", m_header->rate));
        fb.addAttribute(
            new IntAttribute("Mistika/Interlacing", m_header->interlacing));
        fb.addAttribute(new StringAttribute("Mistika/TC", m_header->timeCode));
        fb.addAttribute(
            new StringAttribute("Mistika/Tape", m_header->tapeName));

        fb.addAttribute(new StringAttribute("File", m_filename));
    }

    void MovieMistika::identifiersAtFrame(const ReadRequest& request,
                                          IdentifierVector& ids)
    {
        int frame = request.frame;
        ostringstream str;
        identifier(frame, str);
        ids.resize(1);
        ids.front() = str.str();
    }

    void MovieMistika::identifier(int frame, ostream& o)
    {
        if (frame < m_info.start)
            frame = m_info.start;
        if (frame > m_info.end)
            frame = m_info.end;
        o << frame << ":" << m_filename;
    }

    //----------------------------------------------------------------------

    MovieMistikaIO::MovieMistikaIO()
        : MovieIO("MovieMistika", "v1")
    {
        StringPairVector video;
        StringPairVector audio;
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::AttributeRead;
        addType("js", "Mistika Movie", capabilities, video, audio);
    }

    MovieMistikaIO::~MovieMistikaIO() {}

    std::string MovieMistikaIO::about() const { return "Mistika Movie"; }

    MovieReader* MovieMistikaIO::movieReader() const
    {
        return new MovieMistika();
    }

    MovieWriter* MovieMistikaIO::movieWriter() const { return 0; }

    void MovieMistikaIO::getMovieInfo(const std::string& filename,
                                      MovieInfo&) const
    {
        TWK_THROW_STREAM(IOException, "Unknown usage: " << filename);
    }

} // namespace TwkMovie
