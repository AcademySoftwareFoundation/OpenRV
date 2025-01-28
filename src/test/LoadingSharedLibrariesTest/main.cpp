//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdio>
#include <stdint.h>
#include <iostream>
#include <set>
#include <fstream>
#include <dlfcn.h>

#include <TwkUtil/File.h>
#include <IPCore/AudioRenderer.h>

using namespace std;
using namespace TwkUtil;

// Simple way to communication CLI param with the tests
std::string g_filename;

// The 'LD_LIBRARY_PATH' is mandatory for the RV app under Linux
// let's check it is defined.
TEST_CASE("test definition of the LD_LIBRARY_PATH environment variable")
{
    REQUIRE(getenv("LD_LIBRARY_PATH"));
}

//
// This test verifies the loadability of the `libALSASafeAudioModule.so`
// shared library. We've experienced build system issue on Linux causing
// issue at runtime preventing loading of the shared library.
TEST_CASE("test loading module")
{
    // This is not used, it's only to keep the linker from discarding
    // the IPCore symbols from the library, which are needed the libraries
    // that have dlopen called on them.
    IPCore::AudioRenderer::RendererParameters params =
        IPCore::AudioRenderer::defaultParameters();

    cout << "INFO: Trying to load '" << g_filename.c_str() << "' module"
         << endl;
    void* handle = dlopen(g_filename.c_str(), RTLD_LAZY);
    CHECK(handle);
    if (handle)
    {
        cout << "INFO: Loaded '" << g_filename.c_str() << "' module" << endl;
        dlclose(handle);
    }
    else
    {
        cerr << "ERROR: loading '" << g_filename.c_str()
             << "' module : " << dlerror() << endl;
    }
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(
    4007) // 'function' : must be 'attribute' - see issue #182

int main(int argc, char* argv[])
{
    doctest::Context context(argc, argv);

    // Simple dump parameter parsing.
    // If parameter == '--library' then use next one as a fullpath filename
    for (int paramIndex = 0; paramIndex < argc; paramIndex++)
    {
        std::string param = argv[paramIndex];
        if (param.compare("--library") == 0 && ((paramIndex + 1) < argc))
        {
            g_filename = argv[paramIndex + 1];
            printf("Found filename='%s'\n", g_filename.c_str());
        }
    }

    int doctestResult = context.run();

    // if (context.shouldExit())

    return doctestResult;
}
