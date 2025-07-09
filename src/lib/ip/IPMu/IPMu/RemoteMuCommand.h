//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__RemoteMuParameters__h__
#define __IPCore__RemoteMuParameters__h__

#include <string>
#include <vector>
#include <sstream>
#include <type_traits>
#include <iostream>

#include <IPCore/Session.h>

namespace IPMu
{

    class RemoteMuParameters
    {
    public:
        template <typename... Args> static std::string pack(Args&&... args)
        {
            std::ostringstream oss;
            bool first = true;
            ((first ? (first = false,
                       oss << PackHelper::apply(std::forward<Args>(args)))
                    : (oss << argumentSeparator()
                           << PackHelper::apply(std::forward<Args>(args)))),
             ...);
            return oss.str();
        }

        template <typename... Ts>
        static bool unpack(const std::string& packed, Ts&... outs)
        {
            auto parts = split(packed, argumentSeparator());
            size_t expected = sizeof...(outs);
            if (parts.size() < expected)
                return false;

            size_t i = 0;
            bool success = true;
            auto unpackHelper = [&](auto& out)
            { success &= UnpackHelper::apply(parts[i++], out); };
            (unpackHelper(outs), ...);
            return success;
        }

    private:
        static std::string argumentSeparator() { return ";;"; }

        static std::string arrayItemSeparator() { return ",,"; }

        static std::string arrayArraySeparator() { return "@@"; }

        static std::vector<std::string> split(const std::string& s,
                                              const std::string& sep)
        {
            std::vector<std::string> result;
            size_t pos = 0, prev = 0;
            while ((pos = s.find(sep, prev)) != std::string::npos)
            {
                result.push_back(s.substr(prev, pos - prev));
                prev = pos + sep.size();
            }
            result.push_back(s.substr(prev));
            return result;
        }

        // --- Stringify ---
        static std::string toString(const std::string& s) { return s; }

        static std::string toString(const char* s) { return std::string(s); }

        static std::string toString(bool b) { return b ? "true" : "false"; }

        template <typename T>
        static std::enable_if_t<std::is_arithmetic<T>::value, std::string>
        toString(const T& val)
        {
            return std::to_string(val);
        }

        // --- is_container trait ---
        template <typename T, typename = void>
        struct is_container : std::false_type
        {
        };

        template <typename T>
        struct is_container<T, std::void_t<decltype(std::declval<T>().begin()),
                                           decltype(std::declval<T>().end())>>
            : std::true_type
        {
        };

        template <> struct is_container<std::string> : std::false_type
        {
        };

        // --- Pack Helper ---
        struct PackHelper
        {
            // Scalars
            template <typename T>
            static std::enable_if_t<!is_container<T>::value, std::string>
            apply(const T& val)
            {
                return toString(val);
            }

            // vector<string>
            static std::string apply(const std::vector<std::string>& vec)
            {
                return joinFlat(vec);
            }

            // vector<vector<T>>
            template <typename T>
            static std::string apply(const std::vector<std::vector<T>>& nested)
            {
                std::ostringstream oss;
                bool first = true;
                for (const auto& inner : nested)
                {
                    if (!first)
                        oss << arrayArraySeparator();
                    oss << joinFlat(inner);
                    first = false;
                }
                return oss.str();
            }

            // vector<T>
            template <typename T>
            static std::string apply(const std::vector<T>& flat)
            {
                return joinFlat(flat);
            }

            template <typename T>
            static std::string joinFlat(const std::vector<T>& vec)
            {
                std::ostringstream oss;
                bool first = true;
                for (const auto& val : vec)
                {
                    if (!first)
                        oss << arrayItemSeparator();
                    oss << toString(val);
                    first = false;
                }
                return oss.str();
            }
        };

        // --- Unpack Helper ---
        struct UnpackHelper
        {
            static bool apply(const std::string& s, std::string& out)
            {
                out = s;
                return true;
            }

            static bool apply(const std::string& s, bool& out)
            {
                if (s == "true")
                {
                    out = true;
                    return true;
                }
                if (s == "false")
                {
                    out = false;
                    return true;
                }
                return false;
            }

            static bool apply(const std::string& s, int& out)
            {
                try
                {
                    out = std::stoi(s);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            static bool apply(const std::string& s, float& out)
            {
                try
                {
                    out = std::stof(s);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            static bool apply(const std::string& s, double& out)
            {
                try
                {
                    out = std::stod(s);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            template <typename T>
            static bool apply(const std::string& s, std::vector<T>& out)
            {
                return unpackVector(s, out, arrayItemSeparator());
            }

            template <typename T>
            static bool apply(const std::string& s,
                              std::vector<std::vector<T>>& out)
            {
                out.clear();
                auto chunks = split(s, arrayArraySeparator());
                for (const auto& chunk : chunks)
                {
                    std::vector<T> inner;
                    if (!unpackVector(chunk, inner, arrayItemSeparator()))
                        return false;
                    out.push_back(inner);
                }
                return true;
            }

        private:
            template <typename T>
            static bool unpackVector(const std::string& s, std::vector<T>& out,
                                     const std::string& sep)
            {
                out.clear();
                auto items = split(s, sep);
                for (const auto& item : items)
                {
                    T temp;
                    if (!apply(item, temp))
                        return false;
                    out.push_back(temp);
                }
                return true;
            }
        };
    };

    class RemoteMuCommand
    {
    public:
        RemoteMuCommand(IPCore::Session* s, std::string eventname,
                        std::string parameters = "")
        {
            m_session = s;

            parameters = eventname + "~~" + parameters;

            // Send a single "mu-command" event to be caught by
            // live_review_sync.mu. we hijack the "sender" field to put the
            // event name, sorry about that.
            m_session->userGenericEvent("generic-mu-command",
                                        parameters.c_str(), "");
            m_session->userGenericEvent("push-eat-broadcast-events", "", "");
        }

        ~RemoteMuCommand()
        {
            m_session->userGenericEvent("pop-eat-broadcast-events", "", "");
        }

    private:
        IPCore::Session* m_session;
    };

} // namespace IPMu

#endif
