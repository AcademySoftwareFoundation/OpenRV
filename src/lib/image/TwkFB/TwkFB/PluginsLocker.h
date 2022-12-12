//*****************************************************************************
// Copyright (c) 2020 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************
#ifndef __TwkFB__PluginsLocker__h__
#define __TwkFB__PluginsLocker__h__
#include <TwkFB/dll_defs.h>
#include <pthread.h>
namespace TwkFB {

class TWKFB_EXPORT PluginsLocker {
public:
  PluginsLocker();
  ~PluginsLocker();

  void lock();
  void unlock();

private:
  mutable pthread_mutex_t m_mutex;
};

class TWKFB_EXPORT PluginsLockerGuard {
public:
  explicit PluginsLockerGuard(PluginsLocker *pluginsLocker)
      : m_pluginsLocker(pluginsLocker) {
    if (m_pluginsLocker) {
      m_pluginsLocker->lock();
    }
  }
  ~PluginsLockerGuard() {
    if (m_pluginsLocker) {
      m_pluginsLocker->unlock();
    }
    m_pluginsLocker = nullptr;
  }

private:
  PluginsLocker *m_pluginsLocker;
};

} // TwkFB

#endif // __TwkFB__PluginsLocker__h__