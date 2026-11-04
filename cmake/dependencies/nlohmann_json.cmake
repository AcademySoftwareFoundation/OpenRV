#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FETCHCONTENT_DECLARE(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v${RV_DEPS_NLOHMANN_JSON_VERSION}/json.tar.xz
  URL_HASH MD5=${RV_DEPS_NLOHMANN_JSON_DOWNLOAD_HASH}
  SYSTEM
)
FETCHCONTENT_MAKEAVAILABLE(nlohmann_json)
