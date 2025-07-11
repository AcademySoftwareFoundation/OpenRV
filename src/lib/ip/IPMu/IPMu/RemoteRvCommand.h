//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__RemoteRvCommand__h__
#define __IPCore__RemoteRvCommand__h__

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <type_traits>
#include <IPCore/Session.h>

namespace IPMu
{
    class RemoteRvCommand
    {
    public:
        // Primary constructor - takes command name and arguments directly
        template <typename... Args>
        RemoteRvCommand(IPCore::Session* s, const std::string& command,
                        Args&&... args)
        {
            m_session = s;
            std::string parameters = pack(command, std::forward<Args>(args)...);

            std::cout << "PAYLOAD: " << parameters << std::endl;
            m_session->userGenericEvent("generic-rv-command",
                                        parameters.c_str(), "");

            m_session->userGenericEvent("push-eat-broadcast-events", "");
        }

        ~RemoteRvCommand()
        {
            m_session->userGenericEvent("pop-eat-broadcast-events", "");
        }

        // Static pack method (for cases where you need the JSON without
        // sending)
        template <typename... Args>
        static std::string pack(const std::string& name, Args&&... args)
        {
            std::ostringstream oss;
            oss << "{\"name\":\"" << name << "\",\"args\":[";

            bool first = true;
            ((oss << (first ? (first = false, "") : ",") << formatValue(args)),
             ...);

            oss << "]}";
            return oss.str();
        }

    private:
        IPCore::Session* m_session;

        // Format single values
        static std::string formatValue(const std::string& val)
        {
            return "\"" + escapeString(val) + "\"";
        }

        static std::string formatValue(const char* val)
        {
            return "\"" + escapeString(std::string(val)) + "\"";
        }

        static std::string formatValue(bool val)
        {
            return val ? "true" : "false";
        }

        // Template for all numeric types (int, float, double, etc.)
        template <typename T>
        static typename std::enable_if<std::is_arithmetic<T>::value
                                           && !std::is_same<T, bool>::value,
                                       std::string>::type
        formatValue(T val)
        {
            return std::to_string(val);
        }

        // Format single-level arrays
        static std::string formatValue(const std::vector<std::string>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (const auto& item : val)
            {
                oss << (first ? (first = false, "") : ",") << "\""
                    << escapeString(item) << "\"";
            }
            oss << "]";
            return oss.str();
        }

        // Template for vectors of numeric types (int, float, double, etc.)
        template <typename T>
        static typename std::enable_if<std::is_arithmetic<T>::value
                                           && !std::is_same<T, bool>::value,
                                       std::string>::type
        formatValue(const std::vector<T>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (const auto& item : val)
            {
                oss << (first ? (first = false, "") : ",") << item;
            }
            oss << "]";
            return oss.str();
        }

        static std::string formatValue(const std::vector<bool>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (bool item : val)
            { // Note: bool, not const auto& due to vector<bool> quirks
                oss << (first ? (first = false, "") : ",")
                    << (item ? "true" : "false");
            }
            oss << "]";
            return oss.str();
        }

        // Format nested arrays
        static std::string
        formatValue(const std::vector<std::vector<std::string>>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (const auto& innerVec : val)
            {
                oss << (first ? (first = false, "") : ",")
                    << formatValue(innerVec);
            }
            oss << "]";
            return oss.str();
        }

        // Template for nested vectors of numeric types
        template <typename T>
        static typename std::enable_if<std::is_arithmetic<T>::value
                                           && !std::is_same<T, bool>::value,
                                       std::string>::type
        formatValue(const std::vector<std::vector<T>>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (const auto& innerVec : val)
            {
                oss << (first ? (first = false, "") : ",")
                    << formatValue(innerVec);
            }
            oss << "]";
            return oss.str();
        }

        static std::string
        formatValue(const std::vector<std::vector<bool>>& val)
        {
            std::ostringstream oss;
            oss << "[";
            bool first = true;
            for (const auto& innerVec : val)
            {
                oss << (first ? (first = false, "") : ",")
                    << formatValue(innerVec);
            }
            oss << "]";
            return oss.str();
        }

        // Escape quotes and backslashes in strings
        static std::string escapeString(const std::string& str)
        {
            std::string result;
            for (char c : str)
            {
                if (c == '"' || c == '\\')
                {
                    result += '\\';
                }
                result += c;
            }
            return result;
        }
    };
} // namespace IPMu

#endif
