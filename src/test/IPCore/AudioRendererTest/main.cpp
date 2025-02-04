//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <RvApp/RvGraph.h>
#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/Session.h>
#include <thread>

using namespace std;
using namespace IPCore;

// The 'LD_LIBRARY_PATH' is mandatory for the RV app under Linux
// let's check it is defined.
TEST_CASE("test definition of the LD_LIBRARY_PATH environment variable")
{
    REQUIRE(getenv("LD_LIBRARY_PATH"));
}

TEST_CASE("test creating RendererParameters instance")
{
    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
}

TEST_CASE("test setting default parameters")
{
    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
    AudioRenderer::setDefaultParameters(params);
}

TEST_CASE(
    "test setting default parameters, initiazing & reset (with audio disabled)")
{
    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::initialize();
    // With 'setAudioNever' true the 'reset' method is
    // short-cirtuited, does very little and no module is loaded.
    AudioRenderer::setAudioNever(true);
    AudioRenderer::setNoAudio(true);
    AudioRenderer::reset();
}

TEST_CASE("test setting default parameters & initiazing")
{
    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::initialize();

    AudioRenderer::outputParameters(params);
}

TEST_CASE(
    "test setting default parameters, initiazing & reset (with audio enabled)")
{
    vector<string> files = {"TEST SESSION"};
    Application app = Application();
    Application::instance()->createNewSessionFromFiles(files);

    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::initialize();

    // The reset method is the one ultimately calling the 'loadModule' method.
    // which is the one that dlopen shared library.
    // We need the 'setAudioNever' false otherwise the 'reset' method
    // is short-cirtuited and no module is loaded.
    AudioRenderer::setAudioNever(false);
    AudioRenderer::setNoAudio(false);
    AudioRenderer::reset();

    // There should be 3 entries, meaning that all of them
    // got successfully found and loaded.
    auto modules = AudioRenderer::modules();
    // With disabled ALSA, for now we'll get 2 entries
    CHECK_EQ(3, modules.size());

    bool audioAvailable = true;
    // On Linux, this typically outputs
    // INFO: Found module = 'ALSA (Pre-1.0.14)'
    // INFO: Found module = 'ALSA (Safe)'
    // INFO: Found module = 'Per-Frame'
    for (auto module : modules)
    {
        cout << endl
             << "INFO: Found module = '" << module.name.c_str() << "' " << endl;

        CHECK(module.name.size() > 0);
        cout << "INFO: ... Trying out the '" << module.name.c_str()
             << "' audio module" << endl;

        AudioRenderer::setModule(module.name);

        auto renderer = AudioRenderer::renderer();

        // On CI it is reasonable not to have any audio hardware at all
        if (!renderer)
        {
            audioAvailable &= false;
            CHECK(AudioRenderer::audioDisabled());

            continue;
        }

        auto state = renderer->deviceState();
        cout << "INFO: ... state.device = '" << state.device.c_str() << "'"
             << endl;
        cout << "INFO: ... state.rate = '" << state.rate << "'" << endl;
        cout << "INFO: ... state.latency = '" << state.latency << "'" << endl;
        cout << "INFO: ... state.framesPerBuffer = '" << state.framesPerBuffer
             << "'" << endl;

        CHECK_FALSE(renderer->isPlaying());
        cout << "INFO: ... Trying out play() ..." << endl;
        renderer->play();
        CHECK(renderer->isPlaying());
        CHECK(renderer->isOK());
        cout << "INFO: ... errorString = '" << renderer->errorString() << "'"
             << endl;

        cout << "INFO: ... Trying out stop() ..." << endl;
        renderer->stop();
        CHECK_FALSE(renderer->isPlaying());
        CHECK(renderer->isOK());
        cout << "INFO: ... errorString = '" << renderer->errorString() << "'"
             << endl;

        cout << "INFO: ... Trying out shutdown() ..." << endl;
        renderer->shutdown();
        CHECK(renderer->isOK());
        cout << "INFO: ... errorString = '" << renderer->errorString() << "'"
             << endl;
    }

    if (!audioAvailable)
    {
        CHECK(AudioRenderer::audioDisabledAlways());
    }
}

TEST_CASE("test simple app, start, stop (with audio enabled)")
{
    vector<string> files = {"TEST SESSION"};
    Application app = Application();

    // First session gets 'session0' auto-name
    // At this point we don't expect any session yet
    CHECK(!app.session("session0"));

    auto nodeMgr = IPCore::Application::instance()->nodeManager();
    REQUIRE(nodeMgr);
    auto graph = new Rv::RvGraph(nodeMgr);
    REQUIRE(graph);
    auto session = new Session(graph);

    // Now create a default session and expect to find it.
    CHECK_EQ(session, app.session("session0"));
    app.startPlay(session);
    app.stopAll();

    delete session; // also, deletes m_graph
    // Shouldn't be able to find that session now
    CHECK(!app.session("session0"));
}

#include <chrono>
#include <thread>

using namespace std::this_thread;     // sleep_for
using namespace std::chrono_literals; // ns, us, ms, s, h, etc.

TEST_CASE(
    "test setting default parameters, initiazing & reset (with audio enabled)")
{
    const string testMedia = "bogus_media.bog";
    cout << "INFO: Creating test application ... " << endl;
    Application app = Application();

    cout << "INFO: Setting up audio renderer ... " << endl;
    AudioRenderer::RendererParameters params =
        AudioRenderer::defaultParameters();
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::initialize();
    AudioRenderer::setAudioNever(false);
    AudioRenderer::setNoAudio(false);
    AudioRenderer::reset();
    AudioRenderer::setModule(string("ALSA (Safe)"));

    // First session gets 'session0' auto-name
    // At this point we don't expect any session yet
    CHECK(!app.session("session0"));

    cout << "INFO: Creating test session ... " << endl;
    auto nodeMgr = IPCore::Application::instance()->nodeManager();
    auto graph = new Rv::RvGraph(nodeMgr);
    REQUIRE(graph);
    auto session = new Session(graph);
    REQUIRE(session);
    // As-is, without any Audio IPNode setup,
    // we know we won't have audio enabled in this test :-(
    CHECK(!session->hasAudio());

    session->setFileName(testMedia);
    CHECK_EQ(session->fileName(), testMedia);
    CHECK_EQ(session->filePath(), testMedia);

    cout << "INFO: Starting playback ... " << endl;
    app.startPlay(session);

    session->audioVarLock();
    session->setAudioLoopDuration(100); // arbitrary
    session->setAudioLoopCount(5);      // arbitrary
    session->audioVarUnLock();
    session->playAudio();

    int sleepCount = 10; // approx 1 seconds
    auto renderer = AudioRenderer::renderer();

    // On CI it is reasonable not to have any audio hardware at all
    if (renderer)
    {
        REQUIRE(renderer);
        while (sleepCount > 0)
        {
            cout << "INFO: ... renderer->isOK() = '" << renderer->isOK() << "'"
                 << endl;
            CHECK(renderer->isOK());
            cout << "INFO: ... renderer->isPlaying() = '"
                 << renderer->isPlaying() << "'" << endl;
            // Again, as-is, the test won't have audio :-(
            CHECK(!renderer->isPlaying());
            sleep_for(100ms);
            sleepCount--;
        }
    }

    cout << "INFO: Stopping playback ... " << endl;
    app.stopAll();

    // On CI it is reasonable not to have any audio hardware at all
    if (renderer)
    {
        cout << "INFO: ... renderer->isOK() = '" << renderer->isOK() << "'"
             << endl;
        cout << "INFO: ... renderer->isPlaying() = '" << renderer->isPlaying()
             << "'" << endl;
    }

    cout << "INFO: Cleanup up ..." << endl;
    delete session; // also, deletes m_graph
    // Shouldn't be able to find that session now
    CHECK(!app.session("session0"));
}
