//
//  Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#pragma once

#include <IPCore/Session.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>

#include <string>
#include <vector>
#include <type_traits>

namespace IPMu
{
    class RemoteRvCommand
    {
    public:
        template <typename... Args>
        RemoteRvCommand(IPCore::Session* s, const std::string& command,
                        Args&&... args)
        {
            m_session = s;
            std::string parameters = pack(command, std::forward<Args>(args)...);

            m_session->userGenericEvent("generic-rv-command",
                                        parameters.c_str(), "");

            m_session->userGenericEvent("push-eat-broadcast-events", "");
        }

        ~RemoteRvCommand()
        {
            m_session->userGenericEvent("pop-eat-broadcast-events", "");
        }

        template <typename... Args>
        static std::string pack(const std::string& name, Args&&... args)
        {
            QJsonObject obj;
            obj["name"] = QString::fromStdString(name);

            QJsonArray argsArray;
            (argsArray.append(toJson(args)), ...);
            obj["args"] = argsArray;

            QJsonDocument doc(obj);
            return doc.toJson(QJsonDocument::Compact).toStdString();
        }

    private:
        IPCore::Session* m_session;

        // Ultra-simple conversion - let Qt handle most of the work
        template <typename T> static QJsonValue toJson(T&& val)
        {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
            {
                return QString::fromStdString(val);
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, const char*>)
            {
                return QString(val);
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
            {
                return QJsonValue(val);
            }
            else if constexpr (std::is_integral_v<std::decay_t<T>>)
            {
                return QJsonValue(static_cast<qint64>(val));
            }
            else if constexpr (std::is_floating_point_v<std::decay_t<T>>)
            {
                return QJsonValue(static_cast<double>(val));
            }
            else
            {
                return vectorToJson(val);
            }
        }

        // Handle all vector types generically
        template <typename T>
        static QJsonArray vectorToJson(const std::vector<T>& vec)
        {
            QJsonArray arr;
            for (const auto& item : vec)
            {
                arr.append(toJson(item));
            }
            return arr;
        }
    };
} // namespace IPMu
