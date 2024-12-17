//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <BlackMagicDevices/BlackMagicModule.h>
#include <BlackMagicDevices/DeckLinkProfileCallback.h>
#include <BlackMagicDevices/DeckLinkVideoDevice.h>
#include <TwkUtil/EnvVar.h>
#include <TwkExc/Exception.h>
#ifdef PLATFORM_WINDOWS
#include <String.h>
#endif

#include <map>
#include <sstream>

namespace BlackMagicDevices
{
    using namespace std;

    //
    // Optional env var that can be set to specify the Blackmagic Decklink
    // device profile to use. Note that the device profile directly impacts the
    // number of devices that are visible to RV. Therefore the env var, is
    // specified needs to be taken into account before listing the devices which
    // is why it needs to be handled at the video output module level, not at
    // the device level. Please refer to the Blackmagic Decklink SDK
    // documentation about the possible values that the device profile can take
    // (under the Device Profile section). Example: export
    // TWK_BLACKMAGIC_PROFILE =
    //    "bmdProfileFourSubDevicesHalfDuplex".
    // Note that an another simpler way to select the
    // Blackmagic device profile is to use the Blackmagic Desktop Video Setup
    // app. The Blackmagic device profile selected via this app is persisted
    // between reboot.
    //
    static ENVVAR_STRING(evBlackmagicProfile, "TWK_BLACKMAGIC_PROFILE", "");

    //
    // Ressource allocation is initialization class to handle freeing the
    // resource when going out of scope
    //
    template <typename T> class RAII
    {
    public:
        // Constructor
        RAII(T* ptr)
            : ptr_(ptr)
        {
        }

        // Destructor
        ~RAII()
        {
            if (ptr_)
            {
                ptr_->Release();
            }
        }

        // Delete copy constructor and assignment operator
        RAII(const RAII&) = delete;
        RAII& operator=(const RAII&) = delete;

        // Move constructor and assignment operator
        RAII(RAII&& other) noexcept
            : ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        RAII& operator=(RAII&& other) noexcept
        {
            std::swap(ptr_, other.ptr_);
            return *this;
        }

        // Access
        T* get() const { return ptr_; }

        T*& getRef() { return ptr_; }

    private:
        T* ptr_;
    };

    //
    // Get the DeckLink device iterator interface (OS specific)
    // Note: Will throw an exception if the device iterator interface could not
    // be retrieved
    //
    IDeckLinkIterator* getDeckLinkIterator()
    {
        IDeckLinkIterator* deviceIterator = nullptr;

#if defined(PLATFORM_WINDOWS)
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        HRESULT result =
            CoCreateInstance(CLSID_CDeckLinkIterator, nullptr, CLSCTX_ALL,
                             IID_IDeckLinkIterator, (void**)&deviceIterator);
        if (FAILED(result))
        {
            deviceIterator = nullptr;
        }
#else // #if defined(PLATFORM_WINDOWS)
        deviceIterator = CreateDeckLinkIteratorInstance();
#endif

        if (deviceIterator == nullptr)
        {
            TWK_THROW_EXC_STREAM(
                "Decklink driver failed to initialize or is not installed");
        }

        return deviceIterator;
    }

    //
    // Activate the specified Blackmagic DeckLink device profile
    //
    void setBmdDeviceProfile(const std::string& bmdDeviceProfile)
    {
        static const std::map<std::string, BMDProfileID> bmdProfileStringToId =
            {{"bmdProfileOneSubDeviceFullDuplex",
              bmdProfileOneSubDeviceFullDuplex},
             {"bmdProfileOneSubDeviceHalfDuplex",
              bmdProfileOneSubDeviceHalfDuplex},
             {"bmdProfileTwoSubDevicesFullDuplex",
              bmdProfileTwoSubDevicesFullDuplex},
             {"bmdProfileTwoSubDevicesHalfDuplex",
              bmdProfileTwoSubDevicesHalfDuplex},
             {"bmdProfileFourSubDevicesHalfDuplex",
              bmdProfileFourSubDevicesHalfDuplex}};

        const auto mapIter =
            bmdProfileStringToId.find(evBlackmagicProfile.getValue());

        if (mapIter == bmdProfileStringToId.end())
        {
            std::cerr
                << "ERROR: Unrecognized Blackmagic DeckLink device profile: "
                << bmdDeviceProfile << std::endl;
            return;
        }

        const auto bmdProfileId = mapIter->second;
        int64_t currentProfileIDInt = -1;

        // Note:
        // In order to get the DeckLink device profile currently active, we need
        // an IDeckLinkProfileAttributes interface.
        //
        // In order to set the requested DeckLink device profile, we need an
        // IDeckLinkProfile interface.
        //
        // In order to get an IDeckLinkProfile interface, we need an
        // IDeckLinkProfileManager interface.
        //
        // In order to get an IDeckLinkProfileManager interface, we need an
        // IDeckLink interface.
        //
        // In order to get an IDeckLink interface, we need an IDeckLinkIterator.
        //
        // Please refer to the Blackmagic DeckLink SDK documentation for the
        // details.

        // Retrieve an IDeckLinkIterator interface
        // Note that getDeckLinkIterator() will throw if it fails to retrieve an
        // IDeckLinkIterator interface
        RAII<IDeckLinkIterator> deckLinkIterator(getDeckLinkIterator());

        // Retrieve an IDeckLink interface from the first device (or sub-device)
        // if any
        RAII<IDeckLink> deckLinkDevice(nullptr);
        if (deckLinkIterator.get()->Next(&deckLinkDevice.getRef()) != S_OK)
        {
            std::cerr
                << "ERROR: Could not get the IDeckLink interface to activate "
                   "the requested profile: "
                << bmdDeviceProfile << std::endl;

            return;
        }

        // Get the IDeckLinkProfileAttributes interface if available
        RAII<IDeckLinkProfileAttributes> deckLinkProfileAttributes(nullptr);
        if (deckLinkDevice.get()->QueryInterface(
                IID_IDeckLinkProfileAttributes,
                (void**)&deckLinkProfileAttributes.getRef())
            != S_OK)
        {
            std::cerr << "ERROR: Could not get the IDeckLinkProfileAttributes "
                         "interface "
                         "to activate the requested profile: "
                      << bmdDeviceProfile << std::endl;

            return;
        }

        // Get Profile ID attribute
        if (deckLinkProfileAttributes.get()->GetInt(BMDDeckLinkProfileID,
                                                    &currentProfileIDInt)
            != S_OK)
        {
            std::cerr
                << "ERROR: Could not get the BMDDeckLinkProfileID to activate "
                   "the requested profile: "
                << bmdDeviceProfile << std::endl;

            return;
        }

        // Nothing to do if the requested DeckLinck device profile is already
        // active
        if (currentProfileIDInt == static_cast<int64_t>(bmdProfileId))
        {
            std::cout << "INFO: DeckLink device profile was already activated: "
                      << bmdDeviceProfile << std::endl;

            return;
        }

        // Get the IDeckLinkProfileManager interface if available
        RAII<IDeckLinkProfileManager> deckLinkProfileManager(nullptr);
        if (deckLinkDevice.get()->QueryInterface(
                IID_IDeckLinkProfileManager,
                (void**)&deckLinkProfileManager.getRef())
            != S_OK)
        {
            std::cerr
                << "ERROR: Could not get the IDeckLinkProfileManager interface "
                   "to activate the requested profile: "
                << bmdDeviceProfile << std::endl;

            return;
        }

        // Get the IDeckLinkProfile associated with the specified DeckLink
        // device profile id
        RAII<IDeckLinkProfile> deckLinkProfile(nullptr);
        HRESULT result = deckLinkProfileManager.get()->GetProfile(
            bmdProfileId, &deckLinkProfile.getRef());
        if (result != S_OK || !deckLinkProfile.get())
        {
            std::cerr
                << "ERROR: Could not get the IDeckLinkProfile associated with "
                   "the requested profile: "
                << bmdDeviceProfile << std::endl;

            return;
        }

        // Create profile callback object to determine when we have activated
        // requested profile
        std::shared_ptr<DeckLinkProfileCallback> deckLinkProfileCallback;
        deckLinkProfileCallback =
            std::make_shared<DeckLinkProfileCallback>(deckLinkProfile.get());

        // Register IDeckLinkProfileCallback to monitor profile activation
        if (deckLinkProfileManager.get()->SetCallback(
                deckLinkProfileCallback.get())
            != S_OK)
        {
            std::cerr
                << "ERROR: Could not register the profile callback to monitor "
                   "the activation of the requested profile: "
                << bmdDeviceProfile << std::endl;
            return;
        }

        // Activate Profile
        if (deckLinkProfile.get()->SetActive() != S_OK)
        {
            std::cerr << "ERROR: Could not activate the requested DeckLink "
                         "device profile: "
                      << bmdDeviceProfile << std::endl;

            return;
        }

        // Wait until profile change occurs.
        if (!deckLinkProfileCallback.get()->WaitForProfileActivation())
        {
            std::cerr
                << "ERROR: Timed out waiting for the new profile "
                << bmdDeviceProfile
                << " to be activated. Another application may be delaying the "
                   "profile change."
                << std::endl;
            deckLinkProfileManager.get()->SetCallback(nullptr);
            return;
        }

        deckLinkProfileManager.get()->SetCallback(nullptr);

        // Success if we reached this point
        std::cout << "INFO: DeckLink device profile activated: "
                  << bmdDeviceProfile << std::endl;
    }

    BlackMagicModule::BlackMagicModule(NativeDisplayPtr p)
        : VideoModule()
    {
        open();

        if (!isOpen())
        {
            TWK_THROW_EXC_STREAM("Black Magic: no boards found");
        }
    }

    BlackMagicModule::~BlackMagicModule() { close(); }

    string BlackMagicModule::name() const { return "BlackMagic"; }

    string BlackMagicModule::SDKIdentifier() const
    {
        ostringstream str;
        str << "DeckLink SDK Version "
            << BLACKMAGIC_DECKLINK_API_VERSION_STRING;
        return str.str();
    }

    string BlackMagicModule::SDKInfo() const { return ""; }

    void BlackMagicModule::open()
    {
        if (isOpen())
            return;

        // Select the specified Blackmagic device profile if any was specified
        if (!evBlackmagicProfile.getValue().empty())
        {
            setBmdDeviceProfile(evBlackmagicProfile.getValue());
        }

        RAII<IDeckLinkIterator> deckLinkIterator(getDeckLinkIterator());
        IDeckLink* deckLinkDevice = nullptr;

        while (deckLinkIterator.get()->Next(&deckLinkDevice) == S_OK)
        {
            IDeckLinkOutput* playbackDevice = nullptr;

            // Retrieve the device's name (OS specific)
#if defined(PLATFORM_WINDOWS)
            BSTR name;
            if (deckLinkDevice->GetDisplayName(&name) != S_OK)
            {
                TWK_THROW_EXC_STREAM("Cannot get DeckLink device name");
            }
            wstring nameWString(name, SysStringLen(name));
            string nameString;
            nameString.assign(nameWString.begin(), nameWString.end());
#elif defined(PLATFORM_DARWIN)
            CFStringRef name = nullptr;
            if (deckLinkDevice->GetDisplayName(&name) != S_OK)
            {
                TWK_THROW_EXC_STREAM("Cannot get DeckLink device name");
            }
            char nameChar[256];
            CFStringGetCString(name, nameChar, sizeof(nameChar),
                               kCFStringEncodingMacRoman);
            CFRelease(name);
            string nameString(nameChar);
#else
            char* name = nullptr;
            if (deckLinkDevice->GetDisplayName((const char**)&name) != S_OK)
            {
                TWK_THROW_EXC_STREAM("Cannot get DeckLink device name");
            }
            string nameString(name);
#endif

            if (deckLinkDevice->QueryInterface(IID_IDeckLinkOutput,
                                               (void**)&playbackDevice)
                == S_OK)
            {
                DeckLinkVideoDevice* device = new DeckLinkVideoDevice(
                    this, nameString, deckLinkDevice, playbackDevice);
                if (device->numVideoFormats() != 0)
                {
                    m_devices.push_back(device);
                }
                else
                {
                    delete device;
                    device = nullptr;
                }
            }
            else
            {
                if (playbackDevice != nullptr)
                {
                    playbackDevice->Release();
                    playbackDevice = nullptr;
                }
                if (deckLinkDevice != nullptr)
                {
                    deckLinkDevice->Release();
                    deckLinkDevice = nullptr;
                }
            }
        }

#ifdef PLATFORM_WINDOWS
        glewInit(nullptr);
#endif
    }

    void BlackMagicModule::close()
    {
        for (size_t i = 0; i < m_devices.size(); i++)
            delete m_devices[i];
        m_devices.clear();
    }

    bool BlackMagicModule::isOpen() const { return !m_devices.empty(); }

} // namespace BlackMagicDevices
