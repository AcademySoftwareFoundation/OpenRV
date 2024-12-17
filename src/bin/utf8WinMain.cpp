//*****************************************************************************/
// Copyright (c) 2018 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#if defined(_WIN32)
#include <Windows.h>
#include <shellapi.h>
#endif

int utf8Main(int argc, char* argv[]);

#if defined(_WIN32)
int wmain(int argc, wchar_t* wargv[])
{
    // converts UCS2 arguements to utf-8 charset
    char** argv = new char*[argc];
    for (int argn = 0; argn < argc; argn++)
    {
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wargv[argn], -1, NULL,
                                             0, NULL, NULL);
        if (sizeNeeded > 0)
        {
            char* strTo = new char[sizeNeeded];
            WideCharToMultiByte(CP_UTF8, 0, wargv[argn], -1, strTo, sizeNeeded,
                                NULL, NULL);
            argv[argn] = strTo;
        }
        else
            argv[argn] = nullptr;
    }

    int result = utf8Main(argc, argv);

    for (int argn = 0; argn < argc; argn++)
        delete[] argv[argn];

    delete[] argv;

    return result;
}

extern "C" int APIENTRY WinMain(HINSTANCE /*hInstance*/,
                                HINSTANCE /*hPrevInstance*/,
                                LPSTR /*cmdParamarg*/, int /* cmdShow */)
{
    int argc;
    wchar_t** argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW)
        return -1;
    return wmain(argc, argvW);
}

#else
int main(int argc, char* argv[]) { return utf8Main(argc, argv); }
#endif
