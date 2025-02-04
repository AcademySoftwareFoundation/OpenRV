//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <Tex/Tex.h>
#include <tiffio.h>
#include <stl_ext/string_algo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <string>

using namespace TwkMath;

// The tags.
#define TIFFTAG_CAMERA_MATRIX 33306
#define TIFFTAG_SCREEN_MATRIX 33305

namespace Tex
{

    //******************************************************************************
    void GetMatrices(const char* fileName, Mat44f& camera, Mat44f& screen,
                     bool doTranspose)
    {
        TIFF* tif = TIFFOpen(fileName, "r");
        if (tif == NULL)
        {
            TWK_EXC_THROW_WHAT(Exception, "Could not open file");
        }

        float* ptr = NULL;
        int rslt = TIFFGetField(tif, TIFFTAG_CAMERA_MATRIX, &ptr);
        if (ptr == NULL)
        {
            TWK_EXC_THROW_WHAT(Exception, "Could not get camera matrix");
        }
        else
        {
            memcpy((void*)(&(camera[0][0])), (const void*)ptr,
                   16 * sizeof(float));
        }

        rslt = TIFFGetField(tif, TIFFTAG_SCREEN_MATRIX, &ptr);
        if (ptr == NULL)
        {
            TWK_EXC_THROW_WHAT(Exception, "Could not get screen matrix");
        }
        else
        {
            memcpy((void*)(&(screen[0][0])), (const void*)ptr,
                   16 * sizeof(float));
        }

        TIFFClose(tif);

        if (doTranspose)
        {
            camera.transpose();
            screen.transpose();
        }
    }

    void GetMatrices(const char* fileName, const char* texPath, Mat44f& camera,
                     Mat44f& screen, bool doTranspose)
    {
        assert(fileName);
        assert(texPath);

        if ((strcmp(texPath, "") == 0) || (fileName[0] == '/'))
        {
            GetMatrices(fileName, camera, screen, doTranspose);
        }
        else
        {
            std::vector<std::string> matchedFiles;
            std::vector<std::string> pathdirs;
            stl_ext::tokenize(pathdirs, texPath, ":");
            struct stat statbuf;
            char fullpath[512];
            for (int d = 0; d < pathdirs.size(); ++d)
            {
                snprintf(fullpath, 512, "%s/%s", pathdirs[d].c_str(), fileName);
                if (stat(fullpath, &statbuf) == 0)
                {
                    GetMatrices(fullpath, camera, screen, doTranspose);
                    return;
                }
            }
        }
    }

} // End namespace Tex
