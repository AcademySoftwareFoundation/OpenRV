//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include <vector>

namespace IPCore
{

    class IPNode;

    namespace OCIO = OCIO_NAMESPACE;

    // An OCIO::ConfigIOProxy interface implementation class that is required by
    // OCIOv2 to instantiate a color transfrom from memory (and not from an
    // actual file).
    //
    class ConfigIOProxy : public OCIO::ConfigIOProxy
    {
    public:
        ConfigIOProxy(const IPNode* node)
            : m_node(node) {};
        virtual ~ConfigIOProxy() {};

        // Virtual LUT filepath to indicate that the color transform needs to be
        // initialized from memory using the inTransform.data ByteProperty from
        // the IPNode. Otherwise the LUT is read from the filepath specified.
        static const std::string USE_IN_TRANSFORM_DATA_PROPERTY;

        //
        // OCIO::ConfigIOProxy interface implementation
        //
        std::string getConfigData() const override;
        std::vector<uint8_t> getLutData(const char* filepath) const override;
        std::string getFastLutFileHash(const char* filename) const override;

    private:
        const IPNode* m_node;
    };

} // namespace IPCore
