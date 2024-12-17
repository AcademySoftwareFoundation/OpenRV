//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOmray/IOmray.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <stdlib.h>

namespace TwkFB
{
    using namespace std;

    static bool normalizeOnInput = true;

    IOmray::IOmray()
        : FrameBufferIO()
    {
        unsigned int cap = ImageRead;
        addType("*mraysubfile*", "Mental Ray Stub File Reader", cap);
    }

    IOmray::~IOmray() {}

    string IOmray::about() const
    {
        char temp[80];
        sprintf(temp, "Mental Ray Stub File");
        return temp;
    }

    void IOmray::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        ifstream file(UNICODE_C_STR(filename.c_str()));

        if (!file)
            TWK_THROW_STREAM(Exception, "No file");
        vector<string> buffers(1);

        for (size_t i = 0; file.good(); i++)
        {
            char c = file.get();

            if ((i == 0 && c != 'r') || (i == 1 && c != 'a')
                || (i == 2 && c != 'y') || (i == 4 && c != '.')
                || ((i == 3 || i == 5) && (c < '0' || c > '9')))
            {
                TWK_THROW_STREAM(Exception, "Not an mray stub file");
            }

            if (c == ',')
            {
                buffers.resize(buffers.size() + 1);
            }
            else if (c == 0)
            {
                break;
            }
            else
            {
                buffers.back().append(1, c);
            }
        }

        if (buffers.size() < 8)
        {
            TWK_THROW_STREAM(Exception, "Not an mray sub file");
        }

        fbi.width = atoi(buffers[1].c_str());
        fbi.height = atoi(buffers[2].c_str());
        fbi.uncropWidth = fbi.width;
        fbi.uncropHeight = fbi.height;
        fbi.uncropX = 0;
        fbi.uncropY = 0;
        fbi.pixelAspect = 1;
        fbi.orientation = FrameBuffer::BOTTOMLEFT;
        fbi.proxy.attribute<string>("Mental Ray [7]") = buffers[7];
        fbi.proxy.attribute<string>("Mental Ray [6]") = buffers[6];
        fbi.proxy.attribute<string>("Mental Ray [5]") = buffers[5];
        fbi.proxy.attribute<string>("Mental Ray [4]") = buffers[4];
        fbi.proxy.attribute<string>("Mental Ray [3]") = buffers[3];
        fbi.proxy.attribute<string>("Mental Ray [2]") = buffers[2];
        fbi.proxy.attribute<string>("Mental Ray [1]") = buffers[1];
        fbi.proxy.attribute<string>("Mental Ray Version") = buffers[0];
    }

    void IOmray::readImage(FrameBuffer& fb, const std::string& filename,
                           const ReadRequest& request) const
    {
        FBInfo info;
        getImageInfo(filename, info);
        fb.restructure(info.width, info.height, 0, 1, FrameBuffer::UCHAR);
        info.proxy.copyAttributesTo(&fb);
        // fb.newAttribute("PartialImage", 1.0f);
    }

} //  End namespace TwkFB
