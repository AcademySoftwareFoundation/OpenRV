//
// Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// Unit tests for TwkUtil::CrashHandler's public contract that can be exercised
// without starting Crashpad: the singleton, the not-initialized state, and the
// pre-init annotation buffering path (the init-order-independence guarantee;
// see docs/crash-reporting.md C3). The full crash-capture + symbolication path
// is covered by the opt-in crash-dump smoke test, since it requires actually
// crashing a process.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <TwkUtil/CrashHandler.h>

using TwkUtil::CrashHandler;

TEST_CASE("CrashHandler::instance() is a singleton") { CHECK(&CrashHandler::instance() == &CrashHandler::instance()); }

TEST_CASE("CrashHandler reports the not-initialized contract before initialize()")
{
    // This test never starts Crashpad, so the handler must remain uninitialized
    // and expose no crash-dump directory. (The live-init path is the smoke
    // test's job.)
    const CrashHandler& handler = CrashHandler::instance();
    CHECK_FALSE(handler.isInitialized());
    CHECK(handler.getCrashDumpDirectory().empty());
}

TEST_CASE("addAnnotation before initialize() is safe for mapped and unmapped keys")
{
    CrashHandler& handler = CrashHandler::instance();
    REQUIRE_FALSE(handler.isInitialized());

    // Annotations set before initialize() are buffered (last value per key wins)
    // and only delivered on a successful init, so calling this must never crash
    // regardless of whether the key has a g_annotationMappings[] entry.
    CHECK_NOTHROW(handler.addAnnotation("gpu_vendor", "test-vendor"));   // mapped key (g_annotationMappings[])
    CHECK_NOTHROW(handler.addAnnotation("mu_function", "boom"));         // mapped key
    CHECK_NOTHROW(handler.addAnnotation("not_a_real_key", "value"));     // unmapped key
    CHECK_NOTHROW(handler.addAnnotation("gpu_vendor", "test-vendor-2")); // overwrite (last wins)

    // Buffering annotations must not flip the initialized state.
    CHECK_FALSE(handler.isInitialized());
}
