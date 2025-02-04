//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkCMS__ColorManagementSystem__h__
#define __TwkCMS__ColorManagementSystem__h__
#include <vector>
#include <string>

namespace TwkCMS
{

    //
    //  ColorManagementSystem
    //
    //  In order to declare a ColorManagementSystem, inherit from this
    //  class. In the constructor, add any transforms available at the
    //  time the CMS is instantiated using addTransform().
    //
    //  There are three types of transforms supported: simulation device
    //  transforms, physical device transforms, and combined. In practice,
    //  there isn't any real difference between the types of transforms,
    //  just how they are used.
    //
    //  Physical Device Transform (PhysicalDevice)
    //  ------------------------------------------
    //
    //      The physical device transforms takes data from one of the
    //      defined ColorSpace types and transforms the color to something
    //      that would display correctly for the user. The most obvious
    //      case of this is a monitor profile. The incoming color is
    //      modified so that when displayed on phospher, LCD, or whatever
    //      it approximates most closely the "true" color.
    //
    //      One of the physical devices can be made the default. That is a
    //      hint to the application that this transform should be applied
    //      by default (the user does not have to ask for it). Presumably
    //      the application will allow the user to change it if they
    //      choose.
    //
    //
    //  Simulation Device Transform (SimulationDevice)
    //  ----------------------------------------------
    //
    //      The simulation device transform -- like the physical device
    //      transform -- takes data from one of the defined color spaces
    //      and transforms the color to something that simulates the color
    //      that would result form displaying it on that device. It does
    //      not account for the physical device (like the monitor) that
    //      the user is observing the color on.
    //
    //      One of the simulates devices can be made the default. That is a
    //      hint to the application that this transform should be applied
    //      by default (just like the PhysicalDevice)
    //
    //  Combined Device
    //  ---------------
    //
    //      For some CMSs, it is not possible to separate the
    //      PhysicalDevice and SimulationDevice. In that case you can make
    //      a CombinedDevice.
    //
    //
    //  FAQ:
    //
    //  What if I have a combined profile that shows film-look that goes
    //  directly to the monitor on the desk?
    //
    //      Use a CombinedDevice transform that has them combined
    //      and call it something like "John's Monitor with Film Look". In
    //      this case, don't use the simulation devices. The user is not
    //      forced to use either one of the transforms so doing everything
    //      with the PhysicalDevice transform is ok.
    //
    //  What if my SimulationDevice transform outputs to or inputs from
    //  Lab or some other color space you don't support?
    //
    //      Transform the color into a space that is supported. See
    //      poynton's color FAQ for details. If you have chromaticities,
    //      you can use ILM's Imf library (part of OpenEXR) to convert to
    //      CIEXYZ or use the little CMS library by generating ICC
    //      profiles on the fly.
    //
    //  Can I add transforms after I construct the ColorManagementSystem
    //  object?
    //
    //      Not currently.
    //

    class ColorManagementSystem
    {
    public:
        //
        //  Types
        //

        //
        //  Transform
        //
        //  Each transform should be an instance of a Transform.  You
        //  might, for example, derive a special class of Transform and
        //  make multiple instances of it.
        //
        //  The little CMS implementation makes an ICCProfile class
        //  derived from transform. Each ICCProfile points to a .icc
        //  profile that is opened used then closed by the object.
        //

        class Transform
        {
        public:
            //
            //  ColorSpace
            //
            //  RGB709  : RGB linear light REC 709
            //  CIEXYZ  : CIE XYZ
            //

            enum Type
            {
                SimulationDevice,
                PhysicalDevice,
                CombinedDevice
            };

            enum ColorSpace
            {
                RGB709,
                CIEXYZ
            };

            //
            //  Constructors
            //

            Transform(const std::string& name, Type t,
                      ColorManagementSystem* cms)
                : m_type(t)
                , m_name(name)
                , m_cms(cms)
            {
            }

            virtual ~Transform();

            const std::string& name() const { return m_name; }

            ColorManagementSystem* cms() const { return m_cms; }

            //
            //  default type is PhysicalDevice
            //  default inputSpace is RGB709
            //  default outputSpace is RGB709
            //

            Type type() const { return m_type; }

            virtual ColorSpace inputSpace() const;
            virtual ColorSpace outputSpace() const;

            //
            //  begin() will be called before transform3f(). transform3f()
            //  will be called zero or more times. It will always be
            //  called with num >= 1. end() will be called when the
            //  transform is no longer needed; however, the transform may
            //  be used multiple times during its life
            //  (begin/transform/end sequence may be called multiple
            //  times).
            //

            virtual void begin();
            virtual void transform3f(float* pixelsInInputSpace, size_t num) = 0;
            virtual void end();

        private:
            Type m_type;
            std::string m_name;
            ColorManagementSystem* m_cms;
        };

        typedef std::vector<Transform*> Transforms;
        typedef std::vector<ColorManagementSystem*> CMSPlugins;

        //
        //  Constructors
        //

        ColorManagementSystem(const char* name);
        virtual ~ColorManagementSystem();

        //
        //  API
        //

        const std::string& name() const { return m_name; }

        const Transforms& transforms() const { return m_transforms; }

        Transform* findTransform(const std::string&) const;

        //
        //  Load plugins (NOTE: doesn't actually do anything with them,
        //  that's up to the application)
        //

        static void loadPlugins(const std::string& pluginPath);
        static void addPlugin(ColorManagementSystem* cms);

        static const CMSPlugins& plugins() { return m_plugins; }

        static ColorManagementSystem* findPlugin(const std::string& name);

        //
        //  Some convenient functions
        //  Takes a ':' seperated list of paths and returns the
        //  individual paths in a vector.
        //

        static void splitPath(std::vector<std::string>& tokens,
                              const std::string& path);

    protected:
        //
        //  Call this from derived constructor to add Transforms to the
        //  CMS. Currently we're only interested in two types of
        //  transforms.
        //

        void addTransform(Transform*);

    private:
        std::string m_name;
        Transforms m_transforms;
        static CMSPlugins m_plugins;
    };

} // namespace TwkCMS

#endif // __TwkCMS__ColorManagement__h__
