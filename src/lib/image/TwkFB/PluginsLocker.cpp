//*****************************************************************************
// Copyright (c) 2020 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#include <TwkFB/PluginsLocker.h>

namespace TwkFB {
PluginsLocker::PluginsLocker() { pthread_mutex_init(&m_mutex, 0); }
PluginsLocker::~PluginsLocker() { pthread_mutex_destroy(&m_mutex); }

void PluginsLocker::lock() { pthread_mutex_lock(&m_mutex); }
void PluginsLocker::unlock() { pthread_mutex_unlock(&m_mutex); }
}