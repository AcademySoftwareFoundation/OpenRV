//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <mp4v2Utils/mp4v2Utils.h>
#include <string.h>
#include <sstream>

extern "C"
{
#include <mp4v2/mp4v2.h>
}

namespace mp4v2Utils
{
    using namespace std;

    bool readFile(string filename, void*& fileHandle)
    {
        // Check for MP4v2 Metadata
        MP4LogSetLevel(MP4_LOG_NONE);
        fileHandle = MP4Read(filename.c_str());
        return (fileHandle != MP4_INVALID_FILE_HANDLE);
    }

    bool modifyFile(string filename, void*& fileHandle)
    {
        // Check for MP4v2 Metadata
        MP4LogSetLevel(MP4_LOG_NONE);
        fileHandle = MP4Modify(filename.c_str());
        return (fileHandle != MP4_INVALID_FILE_HANDLE);
    }

    bool closeFile(void*& fileHandle)
    {
        MP4Close(fileHandle);
        return true;
    }

    bool getFOURCC(void* fileHandle, int streamIndex, string& fourcc)
    {
        MP4TrackId id = MP4FindTrackId(fileHandle, streamIndex);
        const char* mdName;

        mdName = MP4GetTrackMediaDataName(fileHandle, id);

        if (mdName == NULL)
            return false;

        fourcc = string(MP4GetTrackMediaDataName(fileHandle, id));

        return true;
    }

    bool assembleAtomName(void* fileHandle, int streamIndex, string atom,
                          string& atomName)
    {
        // Assemble the Atom name
        string fourcc;
        if (!getFOURCC(fileHandle, streamIndex, fourcc))
            return false;
        ostringstream atomStm;
        atomStm << "moov.trak[" << streamIndex << "].mdia.minf.stbl.stsd."
                << fourcc << "." << atom;
        atomName = atomStm.str();

        return true;
    }

    bool getColrType(void* fileHandle, int streamIndex, string& colrType)
    {
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "colr", atomName))
            return false;

        // Skip tracks without COLR
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        // Skip tracks without nclc (for now)
        const char* type;
        string paramType = atomName + ".colorParameterType";
        MP4GetStringProperty(fileHandle, paramType.c_str(), &type);
        colrType = string(type);

        return true;
    }

    bool getNCLCValues(void* fileHandle, int streamIndex, uint64_t& prim,
                       uint64_t& xfer, uint64_t& mtrx)
    {
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "colr", atomName))
            return false;

        // Skip tracks without COLR
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        // Primaries
        string primProp = atomName + ".primariesIndex";
        MP4GetIntegerProperty(fileHandle, primProp.c_str(), &prim);

        // Transfer Function
        string xferProp = atomName + ".transferFunctionIndex";
        MP4GetIntegerProperty(fileHandle, xferProp.c_str(), &xfer);

        // Color Matrix
        string mtrxProp = atomName + ".matrixIndex";
        MP4GetIntegerProperty(fileHandle, mtrxProp.c_str(), &mtrx);

        return true;
    }

    bool getPROFValues(void* fileHandle, int streamIndex,
                       unsigned char*& profile, uint32_t& size)
    {
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "colr", atomName))
            return false;

        // Skip tracks without COLR
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        string icc = atomName + ".iccProfile";
        MP4GetBytesProperty(fileHandle, icc.c_str(), &profile, &size);

        return true;
    }

    bool addColrAtom(void* fileHandle, int streamIndex, uint64_t prim,
                     uint64_t xfer, uint64_t mtrx)
    {
        // Assemble the Atom name
        MP4TrackId id = MP4FindTrackId(fileHandle, streamIndex);

        // Add the atom
        MP4AddColr(fileHandle, id, prim, xfer, mtrx);

        return true;
    }

    bool getAVdnRange(void* fileHandle, int streamIndex, uint64_t& range)
    {
        // Assemble the Atom name
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "ACLR", atomName))
            return false;

        // Skip tracks without AVdn.ACLR
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        // Primaries
        string rangeProp = atomName + ".range";
        MP4GetIntegerProperty(fileHandle, rangeProp.c_str(), &range);

        return true;
    }

    bool getTmcdName(void* fileHandle, int streamIndex, string& reelName)
    {
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "name", atomName))
            return false;

        // XXX Make sure tmcd.name is in the name!

        // Skip files without tmcd.name atom
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        const char* reel;
        string name = atomName + ".name";
        MP4GetStringProperty(fileHandle, name.c_str(), &reel);
        reelName = string(reel);

        return true;
    }

    bool setTmcdName(void* fileHandle, int streamIndex, string reelName)
    {
        string atomName;
        if (!assembleAtomName(fileHandle, streamIndex, "name", atomName))
            return false;

        // Skip files without tmcd.name atom
        if (!MP4HaveAtom(fileHandle, atomName.c_str()))
            return false;

        // Set the tmcd reel name
        string name = atomName + ".name";
        MP4SetStringProperty(fileHandle, name.c_str(), reelName.c_str());

        return true;
    }

} // namespace mp4v2Utils
