//*****************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#ifndef __ZFile__ZFileReader__h__
#define __ZFile__ZFileReader__h__

#include <ZFile/ZFileHeader.h>
#include <iostream>
#include <string>

namespace ZFile
{

    //
    // class Reader
    //
    // Reads a Pixar zfile as output by prman.  If the file was
    // written on a machine with a different byte-order, the Reader
    // will automatically byte-swap all values.
    //

    class Reader
    {
    public:
        Reader();
        ~Reader();

        //
        // Open a file and read the header values.  This
        // must be called before anything else.  Returns
        // false if there was an error reading the file.
        //

        bool setInputFile(const char* fileName);

        //
        // Generic input
        //

        bool setInputStream(std::istream&, const char* name = NULL);

        //
        // This returns non-zero if there was an error in
        // setInputFile() or setInputStream()
        //

        int error() const { return m_error; }

        //
        // Manually close the file if opened with setInputFile().  The
        // file is automatically closed when the Reader is destroyed.
        //

        void closeFile();

        //
        // Has anything been read?
        //

        bool isEmpty() const { return m_empty; }

        //
        // Did the file require byte-swapping
        //

        bool isSwapped() const { return m_swapped; }

        //
        // Access the Header data
        //

        const Header& header() const { return m_header; }

        //
        // This function will cause data to be cached locally in the
        // reader.
        //

        float depthAt(uint16 x, uint16 y) const;

        //
        //  All the data
        //

        const float* data() const { return m_data; }

    private:
        //
        // Read depth values for all pixels into the float array
        // pointed to by "data".
        //

        void readDepths(float* data);

        //
        // Read depth values for "num" number of pixels into
        // the float array pointed to by "data".
        //

        void readDepths(float* data, int num);

        //
        // Read depth values for "stripHeight" number of scanlines
        // into the float array pointed to by "data".
        //

        void readStrip(float* data, int stripHeight);

    private:
        Header m_header;
        std::string m_inName;
        std::istream* m_in;
        float* m_data;
        bool m_needsClosing;
        bool m_swapped;
        bool m_empty;
        int m_error;
    };

    inline float Reader::depthAt(uint16 x, uint16 y) const
    {
        if (x >= m_header.imageWidth)
            x = m_header.imageWidth - 1;
        if (y >= m_header.imageHeight)
            y = m_header.imageHeight - 1;
        // note: you don't have to check for negative since the type is unsigned
        // assert( x < m_header.imageWidth && y < m_header.imageHeight );
        return m_data[y * m_header.imageWidth + x];
    }

} // namespace ZFile

#endif // __ZFile__ZFileReader__h__
