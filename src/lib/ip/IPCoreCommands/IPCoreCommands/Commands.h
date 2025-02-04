//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__Commands__h__
#define __IPCoreCommands__Commands__h__
#include <TwkContainer/Properties.h>
#include <IPCoreCommands/CopyNode.h>
#include <IPCoreCommands/DeleteNode.h>
#include <IPCoreCommands/DeleteProperty.h>
#include <IPCoreCommands/InsertProperty.h>
#include <IPCoreCommands/NewNode.h>
#include <IPCoreCommands/NewProperty.h>
#include <IPCoreCommands/SetInputs.h>
#include <IPCoreCommands/SetProperty.h>
#include <IPCoreCommands/SetPropertyVector.h>
#include <IPCoreCommands/SetViewNode.h>
#include <IPCoreCommands/SetSessionState.h>
#include <IPCoreCommands/SetRangeFromViewNode.h>
#include <IPCoreCommands/WrapExistingNode.h>

#define INIT_FOR_PROPERTY(TYPE)                                             \
    new IPCore::Commands::DeletePropertyInfo<TwkContainer::TYPE>(           \
        "delete" #TYPE);                                                    \
    new IPCore::Commands::SetPropertyInfo<TwkContainer::TYPE>("set" #TYPE); \
    new IPCore::Commands::SetPropertyVectorInfo<TwkContainer::TYPE>(        \
        "setVector" #TYPE);                                                 \
    new IPCore::Commands::InsertPropertyInfo<TwkContainer::TYPE>(           \
        "insert" #TYPE);                                                    \
    new IPCore::Commands::NewPropertyInfo<TwkContainer::TYPE>("new" #TYPE)

#define INIT_IPCORE_COMMANDS                                                   \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setFrame", &IPCore::Session::setFrame,                                \
        &IPCore::Session::currentFrame, TwkApp::CommandInfo::Undoable);        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setFrameEditOnly", &IPCore::Session::setFrame,                        \
        &IPCore::Session::currentFrame, TwkApp::CommandInfo::EditOnly);        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setInc", &IPCore::Session::setInc, &IPCore::Session::inc,             \
        TwkApp::CommandInfo::Undoable);                                        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setIncEditOnly", &IPCore::Session::setInc, &IPCore::Session::inc,     \
        TwkApp::CommandInfo::EditOnly);                                        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setRangeStart", &IPCore::Session::setRangeStart,                      \
        &IPCore::Session::rangeStart, TwkApp::CommandInfo::Undoable);          \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setRangeStartEditOnly", &IPCore::Session::setRangeStart,              \
        &IPCore::Session::rangeStart, TwkApp::CommandInfo::EditOnly);          \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setRangeEnd", &IPCore::Session::setRangeEnd,                          \
        &IPCore::Session::rangeEnd, TwkApp::CommandInfo::Undoable);            \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setRangeEndEditOnly", &IPCore::Session::setRangeEnd,                  \
        &IPCore::Session::rangeEnd, TwkApp::CommandInfo::EditOnly);            \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setInPoint", &IPCore::Session::setInPoint, &IPCore::Session::inPoint, \
        TwkApp::CommandInfo::Undoable);                                        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setInPointEditOnly", &IPCore::Session::setInPoint,                    \
        &IPCore::Session::inPoint, TwkApp::CommandInfo::EditOnly);             \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setOutPoint", &IPCore::Session::setOutPoint,                          \
        &IPCore::Session::outPoint, TwkApp::CommandInfo::Undoable);            \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<int>(                            \
        "setOutPointEditOnly", &IPCore::Session::setOutPoint,                  \
        &IPCore::Session::outPoint, TwkApp::CommandInfo::EditOnly);            \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<double>(                         \
        "setFPS", &IPCore::Session::setFPS, &IPCore::Session::fps,             \
        TwkApp::CommandInfo::Undoable);                                        \
                                                                               \
    new IPCore::Commands::SetSessionStateInfo<double>(                         \
        "setFPSEditOnly", &IPCore::Session::setFPS, &IPCore::Session::fps,     \
        TwkApp::CommandInfo::EditOnly);                                        \
                                                                               \
    new IPCore::Commands::SetSessionObjectStateInfo<std::string>(              \
        "setFileName", &TwkApp::Document::setFileName,                         \
        &TwkApp::Document::fileName, TwkApp::CommandInfo::Undoable);           \
                                                                               \
    new IPCore::Commands::SetSessionObjectStateInfo<std::string>(              \
        "setFileNAmeEditOnly", &TwkApp::Document::setFileName,                 \
        &TwkApp::Document::fileName, TwkApp::CommandInfo::EditOnly);           \
                                                                               \
    new IPCore::Commands::SetRangeFromViewNodeInfo(                            \
        "setRangeFromViewNode", TwkApp::CommandInfo::Undoable);                \
                                                                               \
    new IPCore::Commands::NewNodeInfo();                                       \
    new IPCore::Commands::WrapExistingNodeInfo();                              \
    new IPCore::Commands::CopyNodeInfo();                                      \
    new IPCore::Commands::DeleteNodeInfo();                                    \
    new IPCore::Commands::SetInputsInfo("setInputs",                           \
                                        TwkApp::CommandInfo::Undoable);        \
    new IPCore::Commands::SetInputsInfo("setInputsEditOnly",                   \
                                        TwkApp::CommandInfo::EditOnly);        \
    new IPCore::Commands::SetViewNodeInfo();                                   \
    new TwkApp::MarkerCommandInfo();                                           \
    new TwkApp::HistoryCommandInfo();                                          \
    INIT_FOR_PROPERTY(FloatProperty);                                          \
    INIT_FOR_PROPERTY(HalfProperty);                                           \
    INIT_FOR_PROPERTY(IntProperty);                                            \
    INIT_FOR_PROPERTY(ShortProperty);                                          \
    INIT_FOR_PROPERTY(StringProperty);                                         \
    INIT_FOR_PROPERTY(StringPairProperty);                                     \
    INIT_FOR_PROPERTY(ByteProperty);                                           \
    INIT_FOR_PROPERTY(Vec4fProperty);                                          \
    INIT_FOR_PROPERTY(Vec3fProperty);                                          \
    INIT_FOR_PROPERTY(Vec2fProperty);                                          \
    INIT_FOR_PROPERTY(Vec4hProperty);                                          \
    INIT_FOR_PROPERTY(Vec3hProperty);                                          \
    INIT_FOR_PROPERTY(Vec2hProperty);                                          \
    INIT_FOR_PROPERTY(Mat44fProperty);                                         \
    INIT_FOR_PROPERTY(Mat33fProperty);

namespace IPCore
{
    namespace Commands
    {
        typedef IPCore::Commands::SetSessionState<double> SetFPS;
        typedef IPCore::Commands::SetSessionState<int> SetFrame;
        typedef IPCore::Commands::SetSessionState<int> SetInc;
        typedef IPCore::Commands::SetSessionState<int> SetRangeStart;
        typedef IPCore::Commands::SetSessionState<int> SetRangeEnd;
        typedef IPCore::Commands::SetSessionState<int> SetInPoint;
        typedef IPCore::Commands::SetSessionState<int> SetOutPoint;
        typedef IPCore::Commands::SetSessionObjectState<std::string>
            SetFileName;
    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__Commands__h__
