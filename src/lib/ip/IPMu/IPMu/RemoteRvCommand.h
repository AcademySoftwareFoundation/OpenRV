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

        // Self-test method
        static bool selfTest()
        {
            std::cout << "=== RemoteRvCommand Self-Test ===\n";

            // Test 1: Basic types
            {
                std::string originalStr = "test string";
                int originalInt = 42;
                double originalDouble = 3.14159;
                bool originalBool = true;

                std::string packed =
                    pack("test_basic", originalStr, originalInt, originalDouble,
                         originalBool);
                std::cout << "Test 1 - Basic types JSON: " << packed << "\n";

                // Expected: {"name":"test_basic","args":["test
                // string",42,3.14159,true]}
                if (packed.find("\"name\":\"test_basic\"") == std::string::npos
                    || packed.find("\"test string\"") == std::string::npos
                    || packed.find("42") == std::string::npos
                    || packed.find("true") == std::string::npos)
                {
                    std::cout << "ERROR: Test 1 - Basic types failed\n";
                    return false;
                }
                std::cout << "PASS: Test 1 - Basic types\n";
            }

            // Test 2: Vector<string>
            {
                std::vector<std::string> originalVec = {"apple", "banana",
                                                        "cherry"};
                std::string packed = pack("test_vector_string", originalVec);
                std::cout << "Test 2 - Vector<string> JSON: " << packed << "\n";

                if (packed.find("[\"apple\",\"banana\",\"cherry\"]")
                    == std::string::npos)
                {
                    std::cout << "ERROR: Test 2 - Vector<string> failed\n";
                    return false;
                }
                std::cout << "PASS: Test 2 - Vector<string>\n";
            }

            // Test 3: Vector<int>
            {
                std::vector<int> originalVec = {1, 2, 3, 4, 5};
                std::string packed = pack("test_vector_int", originalVec);
                std::cout << "Test 3 - Vector<int> JSON: " << packed << "\n";

                if (packed.find("[1,2,3,4,5]") == std::string::npos)
                {
                    std::cout << "ERROR: Test 3 - Vector<int> failed\n";
                    return false;
                }
                std::cout << "PASS: Test 3 - Vector<int>\n";
            }

            // Test 4: Nested vectors
            {
                std::vector<std::vector<std::string>> originalNested = {
                    {"a", "b", "c"}, {"1", "2", "3"}};
                std::string packed = pack("test_nested", originalNested);
                std::cout << "Test 4 - Nested vectors JSON: " << packed << "\n";

                if (packed.find("[[\"a\",\"b\",\"c\"],[\"1\",\"2\",\"3\"]]")
                    == std::string::npos)
                {
                    std::cout << "ERROR: Test 4 - Nested vectors failed\n";
                    return false;
                }
                std::cout << "PASS: Test 4 - Nested vectors\n";
            }

            // Test 5: Empty vectors
            {
                std::vector<std::string> emptyVec;
                std::vector<std::vector<int>> emptyNested;
                std::string packed = pack("test_empty", emptyVec, emptyNested);
                std::cout << "Test 5 - Empty vectors JSON: " << packed << "\n";

                if (packed.find("[],[]") == std::string::npos)
                {
                    std::cout << "ERROR: Test 5 - Empty vectors failed\n";
                    return false;
                }
                std::cout << "PASS: Test 5 - Empty vectors\n";
            }

            // Test 6: Mixed types complex test
            {
                std::string str = "test";
                int num = 123;
                bool flag = false;
                std::vector<double> doubles = {1.1, 2.2, 3.3};
                std::vector<std::vector<std::string>> nested = {
                    {"hello", "world"}, {"foo", "bar"}};

                std::string packed =
                    pack("test_mixed", str, num, flag, doubles, nested);
                std::cout << "Test 6 - Mixed types JSON: " << packed << "\n";

                if (packed.find("\"test\"") == std::string::npos
                    || packed.find("123") == std::string::npos
                    || packed.find("false") == std::string::npos
                    || packed.find("[1.1,2.2,3.3]") == std::string::npos)
                {
                    std::cout << "ERROR: Test 6 - Mixed types failed\n";
                    return false;
                }
                std::cout << "PASS: Test 6 - Mixed types\n";
            }

            std::cout << "=== All Tests Passed! ===\n";
            return true;
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
