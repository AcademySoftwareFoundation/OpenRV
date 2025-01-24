//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <OCIONodes/ConfigIOProxy.h>
#include <TwkExc/Exception.h>
#include <IPCore/IPNode.h>

namespace IPCore
{

    const std::string ConfigIOProxy::USE_IN_TRANSFORM_DATA_PROPERTY =
        "OCIOIPNode.inTransform.data.icc";

    std::string ConfigIOProxy::getConfigData() const
    {
        std::ostringstream os;
        OCIO::Config::CreateRaw()->serialize(os);
        return os.str();
    }

    std::vector<uint8_t> ConfigIOProxy::getLutData(const char* filepath) const
    {
        std::vector<uint8_t> buffer;

        // Load color transform from memory
        if (std::string(filepath).find(
                ConfigIOProxy::USE_IN_TRANSFORM_DATA_PROPERTY)
            != std::string::npos)
        {
            const TwkContainer::ByteProperty* inTransformData =
                m_node->property<TwkContainer::ByteProperty>(
                    "inTransform.data");
            if (!inTransformData)
            {
                std::ostringstream os;
                os << "Could not read property inTransform.data from : "
                   << m_node->name();
                TWK_THROW_EXC_STREAM(os.str().c_str());
            }
            const char* data =
                static_cast<const char*>(inTransformData->rawData());
            const size_t dataSize =
                inTransformData
                    ->size(); // don't assume null terminated (ICC is not text)
            buffer.resize(dataSize);
            std::memcpy(buffer.data(), data, dataSize);
        }
        // Load color transform from file
        else
        {
            std::ifstream fstream(filepath, std::ios_base::in
                                                | std::ios_base::binary
                                                | std::ios::ate);
            if (fstream.fail())
            {
                std::ostringstream os;
                os << "Could not read LUT file : " << filepath;
                TWK_THROW_EXC_STREAM(os.str().c_str());
            }

            const auto eofPosition =
                static_cast<std::streamoff>(fstream.tellg());
            buffer.resize(eofPosition);
            fstream.seekg(0, std::ios::beg);
            fstream.read(reinterpret_cast<char*>(buffer.data()), eofPosition);
        }

        return buffer;
    }

    std::string ConfigIOProxy::getFastLutFileHash(const char* filename) const
    {
        return std::string(filename);
    }

} // namespace IPCore
