//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Action__h__
#define __TwkApp__Action__h__
#include <string>

namespace TwkApp
{
    class Event;
    class Document;

    ///
    /// class Action
    ///
    /// This is a base class for actions which can be bound to menu items
    /// or event table entries. The base class does nothing. In order to
    /// use an action there needs to be a sub-class of Action which does
    /// something specific.
    ///
    /// For example, you could have something like this for a Python
    /// action:
    ///
    ///     class PythonAction : public Action
    ///     {
    ///     public:
    ///         PythonAction(string& pythonCommand);
    ///         virtual void execute(const Event&);
    ///     }
    ///
    /// In this case, execute() would be implemented to evaluate the
    /// pythonCommand argument using the python interpreter.
    ///
    /// You must override the copy() function!
    ///

    class Action
    {
    public:
        Action() {}

        Action(const std::string& help)
            : m_docstring(help)
        {
        }

        virtual ~Action();
        virtual void execute(Document*, const Event&) const;
        virtual Action* copy() const = 0;
        virtual bool error() const = 0;

        const std::string& docString() const { return m_docstring; }

    private:
        std::string m_docstring;
    };

} // namespace TwkApp

#endif // __TwkApp__Action__h__
