//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <RvApp/RvGraph.h>
#include <IPCore/Application.h>
#include <IPCore/Session.h>
#include <IPCore/Application.h>

using namespace std;
using namespace IPCore;

TEST_CASE("test creating IPCore::Application instance")
{
    Application app = Application();
    REQUIRE(Application::instance());
    REQUIRE(Application::instance() == &app);
    REQUIRE(App() == Application::instance());
}

TEST_CASE("test creating test session")
{
    vector<string> files = {"TEST SESSION"};
    Application app = Application();
    Application::instance()->createNewSessionFromFiles(files);
}

TEST_CASE("test get session by name")
{
    const string testSessionName = "test session";

    cout << "INFO: Creating test application ... " << endl;
    Application app = Application();

    // At this point we don't expect any session yet
    CHECK(!app.session(testSessionName));

    cout << "INFO: Creating test session ... " << endl;
    auto session = new Session();
    REQUIRE(session);
    cout << "INFO: Trying to get session by name ..." << endl;
    // Won't find it just yet
    CHECK(!app.session(testSessionName));
    // But will find default name ...
    CHECK_EQ(session->name(), "session0");
    CHECK(app.session("session0"));

    cout << "INFO: Changing session name and trying again ..." << endl;
    session->setName(testSessionName);
    CHECK_EQ(session->name(), testSessionName);
    CHECK(app.session(testSessionName));

    cout << "INFO: Cleanup up ..." << endl;
    delete session; // also, deletes m_graph
    // Shouldn't be able to find that session now
    CHECK(!app.session(testSessionName));
}

TEST_CASE("test creating session with node graph")
{
    const string testSessionName = "test session";

    cout << "INFO: Creating test application ... " << endl;
    Application app = Application();

    // At this point we don't expect any session yet
    CHECK(!app.session(testSessionName));

    cout << "INFO: Creating test session ... " << endl;
    auto nodeMgr = IPCore::Application::instance()->nodeManager();
    auto graph = new Rv::RvGraph(nodeMgr);
    REQUIRE(graph);
    auto session = new Session(graph);
    REQUIRE(session);

    cout << "INFO: Cleanup up ..." << endl;
    delete session; // also, deletes m_graph
    // Shouldn't be able to find that session now
    CHECK(!app.session(testSessionName));
}
