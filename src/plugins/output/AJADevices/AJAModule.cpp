//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <AJADevices/AJAModule.h>
#include <AJADevices/KonaVideoDevice.h>
#include <TwkExc/Exception.h>
#include <TwkGLF/GLFBO.h>
#ifdef PLATFORM_DARWIN
#include <TwkGLF/GL.h>
#endif
#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#endif
#include <sstream>
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"

namespace AJADevices
{
  using namespace std;

  AJAModule::AJAModule( NativeDisplayPtr p, unsigned int appID,
                        OperationMode mode )
      : VideoModule(), m_devicescan( 0 ), m_mode( mode ), m_appID( appID )
  {
    m_devicescan = new CNTV2DeviceScanner();
    // cout << "m_devicescan: " << hex << m_devicescan << endl;
    // cout << "opening AJA ...." << endl;
    open();

    if( !isOpen() )
    {
      TWK_THROW_EXC_STREAM( "AJA: no devices found" );
    }
  }

  AJAModule::~AJAModule()
  {
    close();
    delete reinterpret_cast<CNTV2DeviceScanner*>( m_devicescan );
  }

  string AJAModule::name() const
  {
    return m_mode == OperationMode::SimpleMode ? "AJA (Control Panel)" : "AJA";
  }

  string AJAModule::SDKIdentifier() const
  {
    ostringstream str;
    str << "AJA NTV2 SDK Version " << AJA_NTV2_SDK_VERSION_MAJOR << "."
        << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT;
    return str.str();
  }

  string AJAModule::SDKInfo() const
  {
    return "";
  }

  void AJAModule::open()
  {
    if( isOpen() ) return;

    reinterpret_cast<CNTV2DeviceScanner*>( m_devicescan )->ScanHardware();

    const NTV2DeviceInfoList& deviceList =
        reinterpret_cast<CNTV2DeviceScanner*>( m_devicescan )
            ->GetDeviceInfoList();

    for( size_t i = 0; i < deviceList.size(); i++ )
    {
      const NTV2DeviceInfo& info = deviceList[i];
      // cout << "info.deviceIdentifier: " << info.deviceIdentifier << " Mode: "
      // << m_mode << endl;
      m_devices.push_back(
          new KonaVideoDevice( this, info.deviceIdentifier, i, m_appID,
                               (KonaVideoDevice::OperationMode)m_mode ) );
    }
  }

  void AJAModule::close()
  {
    for( size_t i = 0; i < m_devices.size(); i++ ) delete m_devices[i];
    m_devices.clear();
  }

  bool AJAModule::isOpen() const
  {
    return !m_devices.empty();
  }

}  // namespace AJADevices
