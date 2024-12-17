//*****************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#include <ZFile/ZFileReader.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <string.h>

namespace ZFile
{

    //*****************************************************************************
    //
    // Helper routines for byte-swapping
    //

    template <class T> static void swapValInPlace(T& inVal)
    {
        size_t size = sizeof(T);
        T copyVal = inVal;

        char* inPtr = reinterpret_cast<char*>(&inVal);
        char* copyPtr = (reinterpret_cast<char*>(&copyVal)) + size - 1;

        for (int i = 0; i < size; ++i)
        {
            *inPtr++ = *copyPtr--;
        }
    }

    static void swapFloatData(float* data, int num)
    {
        struct bytes
        {
            char c[4];
        };

        bytes* ip = reinterpret_cast<bytes*>(data);

        for (int i = 0; i < num; i++)
        {
            bytes temp = ip[i];
            ip[i].c[0] = temp.c[3];
            ip[i].c[1] = temp.c[2];
            ip[i].c[2] = temp.c[1];
            ip[i].c[3] = temp.c[0];
        }
    }

    //*****************************************************************************
    //
    // Reader methods
    //

    Reader::Reader()
    {
        m_header.magicNumber = Header::Magic;
        m_header.imageWidth = 0;
        m_header.imageHeight = 0;

        size_t matSize = sizeof(Matrix44);

        memset(m_header.worldToScreen, 0, matSize);
        memset(m_header.worldToCamera, 0, matSize);

        m_in = NULL;
        m_needsClosing = false;
        m_error = 0;
        m_swapped = false;
        m_data = NULL;
        m_empty = true;
    }

    Reader::~Reader()
    {
        closeFile();
        delete[] m_data;
    }

    bool Reader::setInputFile(const char* fileName)
    {
        std::ifstream* file = new std::ifstream(UNICODE_C_STR(fileName));

        if (!(*file))
        {
            m_error = 1;
            return false;
        }

        m_needsClosing = true;
        return setInputStream(*file, fileName);
    }

    bool Reader::setInputStream(std::istream& in, const char* name)
    {
        m_inName = name;
        m_in = &in;

        //
        // Read all the header fields in one-by-one
        // since they aren't all word aligned.
        //

        m_in->read((char*)&m_header.magicNumber, sizeof(m_header.magicNumber));
        m_in->read((char*)&m_header.imageWidth, sizeof(m_header.imageWidth));
        m_in->read((char*)&m_header.imageHeight, sizeof(m_header.imageHeight));
        m_in->read((char*)&m_header.worldToScreen,
                   sizeof(m_header.worldToScreen));
        m_in->read((char*)&m_header.worldToCamera,
                   sizeof(m_header.worldToCamera));

        //
        // Check if this file needs byte-swapping
        //

        if (m_header.magicNumber == Header::Cigam)
        {
            m_swapped = true;
            swapValInPlace(m_header.magicNumber);
            swapValInPlace(m_header.imageWidth);
            swapValInPlace(m_header.imageHeight);
            swapFloatData(m_header.worldToScreen, 16);
            swapFloatData(m_header.worldToCamera, 16);
        }
        else if (m_header.magicNumber != Header::Magic)
        {
            m_error = 1;
            return false;
        }

        if (m_data)
        {
            delete[] m_data;
            m_data = NULL;
        }
        m_data = new float[m_header.imageWidth * m_header.imageHeight];
        readDepths(m_data);

        m_empty = false;
        return true;
    }

    void Reader::closeFile()
    {
        if (m_in && m_needsClosing)
        {
            delete m_in;
            m_in = NULL;
        }
    }

    void Reader::readDepths(float* data)
    {
        int num = m_header.imageWidth * m_header.imageHeight;
        size_t size = sizeof(float) * num;
        m_in->read((char*)data, size);
        if (m_swapped)
        {
            swapFloatData(data, num);
        }
    }

    void Reader::readDepths(float* data, int num)
    {
        size_t size = sizeof(float) * num;
        m_in->read((char*)data, size);
        if (m_swapped)
        {
            swapFloatData(data, num);
        }
    }

    void Reader::readStrip(float* data, int stripHeight)
    {
        int num = m_header.imageWidth * stripHeight;
        size_t size = sizeof(float) * num;
        m_in->read((char*)data, size);
        if (m_swapped)
        {
            swapFloatData(data, num);
        }
    }

} // namespace ZFile
