//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetIntSessionState__h__
#define __IPCoreCommands__SetIntSessionState__h__
#include <TwkApp/Command.h>
#include <IPCore/Session.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  Set values on the session.
        //

        template <typename ValueType>
        class SetSessionState : public TwkApp::Command
        {
        public:
            SetSessionState(const TwkApp::CommandInfo* info)
                : TwkApp::Command(info)
                , m_session(0)
                , m_value(ValueType(0))
            {
            }

            virtual ~SetSessionState() {}

            void setArgs(IPCore::Session* session, ValueType value)
            {
                m_session = session;
                m_value = value;
            }

            ValueType get() const;
            void set(ValueType) const;

            virtual void doit()
            {
                ValueType v = m_value;
                m_value = get();
                set(v);
            }

        private:
            Session* m_session;
            ValueType m_value;
        };

        template <typename ValueType>
        class SetSessionStateInfo : public TwkApp::CommandInfo
        {
        public:
            typedef void (IPCore::Session::*SetFunction)(ValueType);
            typedef ValueType (IPCore::Session::*GetFunction)() const;
            typedef TwkApp::Command Command;

            SetSessionStateInfo(const std::string& name, SetFunction setF,
                                GetFunction getF,
                                TwkApp::CommandInfo::UndoType type)
                : CommandInfo(name, type)
                , m_setFunction(setF)
                , m_getFunction(getF)
            {
            }

            virtual ~SetSessionStateInfo() {}

            virtual Command* newCommand() const;

            SetFunction m_setFunction;
            GetFunction m_getFunction;
        };

        template <typename ValueType>
        ValueType SetSessionState<ValueType>::get() const
        {
            const SetSessionStateInfo<ValueType>* i =
                static_cast<const SetSessionStateInfo<ValueType>*>(info());
            return (m_session->*(i->m_getFunction))();
        }

        template <typename ValueType>
        void SetSessionState<ValueType>::set(ValueType v) const
        {
            const SetSessionStateInfo<ValueType>* i =
                static_cast<const SetSessionStateInfo<ValueType>*>(info());
            (m_session->*(i->m_setFunction))(v);
        }

        template <typename ValueType>
        TwkApp::Command* SetSessionStateInfo<ValueType>::newCommand() const
        {
            return new SetSessionState<ValueType>(this);
        }

        //----------------------------------------------------------------------

        //
        //  Set values on the session.
        //

        template <typename ValueType>
        class SetSessionObjectState : public TwkApp::Command
        {
        public:
            SetSessionObjectState(const TwkApp::CommandInfo* info)
                : TwkApp::Command(info)
                , m_session(0)
            {
            }

            virtual ~SetSessionObjectState() {}

            void setArgs(IPCore::Session* session, const ValueType& value)
            {
                m_session = session;
                m_value = value;
            }

            const ValueType& get() const;
            void set(const ValueType&) const;

            virtual void doit()
            {
                ValueType v = m_value;
                m_value = get();
                set(v);
            }

        private:
            Session* m_session;
            ValueType m_value;
        };

        template <typename ValueType>
        class SetSessionObjectStateInfo : public TwkApp::CommandInfo
        {
        public:
            typedef void (IPCore::Session::*SetFunction)(const ValueType&);
            typedef const ValueType& (IPCore::Session::*GetFunction)() const;
            typedef TwkApp::Command Command;

            SetSessionObjectStateInfo(const std::string& name, SetFunction setF,
                                      GetFunction getF,
                                      TwkApp::CommandInfo::UndoType type)
                : CommandInfo(name, type)
                , m_setFunction(setF)
                , m_getFunction(getF)
            {
            }

            virtual ~SetSessionObjectStateInfo() {}

            virtual Command* newCommand() const;

            SetFunction m_setFunction;
            GetFunction m_getFunction;
        };

        template <typename ValueType>
        const ValueType& SetSessionObjectState<ValueType>::get() const
        {
            const SetSessionObjectStateInfo<ValueType>* i =
                static_cast<const SetSessionObjectStateInfo<ValueType>*>(
                    info());
            return (m_session->*(i->m_getFunction))();
        }

        template <typename ValueType>
        void SetSessionObjectState<ValueType>::set(const ValueType& v) const
        {
            const SetSessionObjectStateInfo<ValueType>* i =
                static_cast<const SetSessionObjectStateInfo<ValueType>*>(
                    info());
            (m_session->*(i->m_setFunction))(v);
        }

        template <typename ValueType>
        TwkApp::Command*
        SetSessionObjectStateInfo<ValueType>::newCommand() const
        {
            return new SetSessionObjectState<ValueType>(this);
        }

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetIntSessionObjectState__h__
