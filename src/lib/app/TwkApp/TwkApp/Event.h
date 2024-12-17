//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Event__h__
#define __TwkApp__Event__h__
#include <iostream>
#include <vector>
#include <map>

namespace TwkApp
{
    class VideoDevice;
    class EventNode;
    class EventTable;

    ///
    ///  class Event
    ///
    ///  This is a base class. To actually create an event, you'll probably
    ///  use a sub-class of the Event class. The most obvious uses are the
    ///  KeyEvent and PointerEvent (mouse) classes.
    ///
    ///  Special events like RedrawEvent are also sub-classed from this
    ///  event.
    ///

    class Event
    {
    public:
        typedef std::vector<std::string> StringVector;

        //
        //  It seems dangerous to allow events with no sender and blank name
        //  (caused a crash in one corner case), so for now we disallow all such
        //  events.
        //
        // Event() : m_name(""), m_sender(0) {}

        Event(const std::string& name, const EventNode* sender, void* data = 0)
            : m_name(name)
            , m_sender(sender)
            , m_data(data)
            , m_table(0)
            , handled(false)
        {
        }

        const std::string& name() const { return m_name; }

        const EventNode* sender() const { return m_sender; }

        const EventTable* eventTable() const { return m_table; }

        virtual void output(std::ostream&) const;

        void* data() const { return m_data; }

        double timeStamp() const { return m_timeStamp; }

        mutable bool handled;

    private:
        std::string m_name;
        const EventNode* m_sender;
        void* m_data;
        mutable const EventTable* m_table;
        mutable double m_timeStamp;

        friend class EventNode;
        friend class Document;
    };

    //
    //  For debugging purposes.
    //

    inline std::ostream& operator<<(std::ostream& o, const Event& e)
    {
        e.output(o);
        return o;
    }

    //----------------------------------------------------------------------
    ///
    /// RenderEvent
    ///
    /// A RenderEvent does not necessarily correspond to a device, but
    /// can be generated in order to allow binding a function for
    /// rendering.
    ///
    /// The additional string field is meant to provide context if any is
    /// needed.
    ///

    class RenderEvent : public Event
    {
    public:
        // RenderEvent() : Event() {}
        RenderEvent(const std::string& name, const EventNode* sender, int w,
                    int h, void* data = 0)
            : m_w(w)
            , m_h(h)
            , m_device(0)
            , Event(name, sender, data)
        {
        }

        RenderEvent(const std::string& name, const EventNode* sender, int w,
                    int h, const std::string& contents, void* data = 0)
            : m_w(w)
            , m_h(h)
            , m_device(0)
            , m_stringContent(contents)
            , Event(name, sender, data)
        {
        }

        RenderEvent(const std::string& name, const EventNode* sender,
                    const VideoDevice* device, int w, int h, void* data = 0)
            : m_w(w)
            , m_h(h)
            , m_device(device)
            , Event(name, sender, data)
        {
        }

        RenderEvent(const std::string& name, const EventNode* sender,
                    const VideoDevice* device, int w, int h,
                    const std::string& contents, void* data = 0)
            : m_w(w)
            , m_h(h)
            , m_device(device)
            , m_stringContent(contents)
            , Event(name, sender, data)
        {
        }

        int w() const { return m_w; }

        int h() const { return m_h; }

        const VideoDevice* device() const { return m_device; }

        const std::string& stringContent() const { return m_stringContent; }

    private:
        int m_w;
        int m_h;
        std::string m_stringContent;
        const VideoDevice* m_device;
    };

    //----------------------------------------------------------------------
    ///
    ///  RenderContextChangeEvent
    ///
    ///  Sent if a render context is about to change.
    ///

    class RenderContextChangeEvent : public Event
    {
    public:
        // RenderContextChangeEvent() : Event() {}
        RenderContextChangeEvent(const std::string& name,
                                 const EventNode* sender, void* data = 0)
            : Event(name, sender, data)
        {
        }
    };

    //----------------------------------------------------------------------
    ///
    ///  VideoDeviceContextChangeEvent
    ///
    ///  Sent if a physical device associated with another device
    ///  changes. E.g. if a GL video device (window) is moved from one
    ///  monitor (physical device) to another.
    ///

    class VideoDeviceContextChangeEvent : public Event
    {
    public:
        // VideoDeviceContextChangeEvent() : Event() {}
        VideoDeviceContextChangeEvent(const std::string& name,
                                      const EventNode* sender,
                                      const VideoDevice* device,
                                      const VideoDevice* physicalDevice,
                                      void* data = 0)
            : Event(name, sender, data)
            , m_device(device)
            , m_physicalDevice(physicalDevice)
        {
        }

        const VideoDevice* m_device;
        const VideoDevice* m_physicalDevice;
    };

    //----------------------------------------------------------------------
    ///
    ///  ActivityChangeEvent
    ///
    ///  User activity level changed
    ///

    class ActivityChangeEvent : public Event
    {
    public:
        // ActivityChangeEvent() : Event() {}
        ActivityChangeEvent(const std::string& name, const EventNode* sender,
                            void* data = 0)
            : Event(name, sender, data)
        {
        }
    };

    //----------------------------------------------------------------------
    ///
    ///  ModifierEvent
    ///
    ///  A ModifierEvent is a base class which holds the state of modifers
    ///  for Key and Mouse events (amoung others).
    ///

    class ModifierEvent : public Event
    {
    public:
        enum Modifier
        {
            None = 0,
            Shift = 1 << 0,
            Control = 1 << 1,
            Alt = 1 << 2,
            Meta = 1 << 3,
            Super = 1 << 4,
            CapLock = 1 << 5,
            NumLock = 1 << 6,
            ScrollLock = 1 << 7
        };

        // ModifierEvent() : Event(), m_modifiers(0) {}

        ModifierEvent(const std::string& name, const EventNode* sender,
                      unsigned int modifiers, void* data = 0)
            : Event(name, sender, data)
            , m_modifiers(modifiers)
        {
        }

        bool modifierState(Modifier m) const
        {
            return (unsigned int)m & m_modifiers;
        }

        unsigned int modifiers() const { return m_modifiers; }

        virtual void output(std::ostream&) const;

    private:
        unsigned int m_modifiers;
    };

    //----------------------------------------------------------------------
    ///
    ///  KeyEvent is a base class for Key press and release events.
    ///

    class KeyEvent : public ModifierEvent
    {
    public:
        KeyEvent(const std::string& name, const EventNode* sender,
                 unsigned int key, unsigned int modifiers, void* data = 0)
            : ModifierEvent(name, sender, modifiers, data)
            , m_key(key)
        {
        }

        unsigned int key() const { return m_key; }

        virtual void output(std::ostream&) const;

    private:
        unsigned int m_key;
    };

    //----------------------------------------------------------------------
    ///
    ///  KeyPressEvent / KeyReleaseEvent
    ///
    ///  These events correspond to the press and release of a key. The
    ///  information returned is the same for each. Note that the modifiers
    ///  can change between KeyPress and KeyRelease.
    ///

    class KeyPressEvent : public KeyEvent
    {
    public:
        KeyPressEvent(const std::string& name, const EventNode* sender,
                      unsigned int key, unsigned int modifiers, void* data = 0)
            : KeyEvent(name, sender, key, modifiers, data)
        {
        }
    };

    class KeyReleaseEvent : public KeyEvent
    {
    public:
        KeyReleaseEvent(const std::string& name, const EventNode* sender,
                        unsigned int key, unsigned int modifiers,
                        void* data = 0)
            : KeyEvent(name, sender, key, modifiers, data)
        {
        }
    };

    //----------------------------------------------------------------------
    ///
    ///  PointerEvent
    ///
    ///  Your basic mouse/touchpad/trackball event. The event includes the
    ///  domain -- the values which the pointer can legally have. The event
    ///  will be sent anytime the pointer position changes. When a button
    ///  press/release event occurs the sub-class PointerButtonPressEvent
    ///  and PointerButtonReleaseEvent will be sent instead. Note that this
    ///  even has the button state information already present.
    ///

    class PointerEvent : public ModifierEvent
    {
    public:
        // PointerEvent() : ModifierEvent(),
        //                  m_x(0), m_y(0), m_w(0), m_h(0), m_px(0), m_py(0),
        //                  m_buttonStates(0) {}

        PointerEvent(const std::string& name, const EventNode* sender,
                     unsigned int modifiers, int x, int y, /// position
                     int w, int h,                         /// size of domain
                     int ox, int oy,                       /// "push" values
                     unsigned int buttonStates, void* data = 0)
            : ModifierEvent(name, sender, modifiers, data)
            , m_x(x)
            , m_y(y)
            , m_w(w)
            , m_h(h)
            , m_px(ox)
            , m_py(oy)
            , m_buttonStates(buttonStates)
        {
        }

        int x() const { return m_x; }

        int y() const { return m_y; }

        int w() const { return m_w; }

        int h() const { return m_h; }

        int startX() const { return m_px; }

        int startY() const { return m_py; }

        unsigned int buttonStates() const { return m_buttonStates; }

        virtual void output(std::ostream&) const;

    private:
        int m_x;
        int m_y;
        int m_w;
        int m_h;
        int m_px;
        int m_py;
        unsigned int m_buttonStates;
    };

    //----------------------------------------------------------------------
    ///
    ///  PointerButtonPressEvent
    ///
    ///  This event corresponds to a button on a mouse-like device being
    ///  pressed.
    ///

    class PointerButtonPressEvent : public PointerEvent
    {
    public:
        PointerButtonPressEvent(const std::string& name,
                                const EventNode* sender, unsigned int modifiers,
                                int x, int y, int w, int h, int ox, int oy,
                                unsigned int buttonStates, void* data = 0,
                                float aTime = 0.0)
            : PointerEvent(name, sender, modifiers, x, y, w, h, ox, oy,
                           buttonStates, data)
            , m_activationTime(aTime)
        {
        }

        float activationTime() const { return m_activationTime; }

    private:
        float m_activationTime;
    };

    //----------------------------------------------------------------------
    ///
    ///  PointerButtonReleaseEvent
    ///
    ///  This event corresponds to a button on a mouse-like device being
    ///  released.
    ///

    class PointerButtonReleaseEvent : public PointerEvent
    {
    public:
        PointerButtonReleaseEvent(const std::string& name,
                                  const EventNode* sender,
                                  unsigned int modifiers, int x, int y, int w,
                                  int h, int ox, int oy,
                                  unsigned int buttonStates, void* data = 0)
            : PointerEvent(name, sender, modifiers, x, y, w, h, ox, oy,
                           buttonStates, data)
        {
        }
    };

    //----------------------------------------------------------------------
    ///
    /// DragAndDropEvent
    ///
    /// This event series is sent when a drag 'n drop is occuring. Its
    /// similar to a PointerEvent. The contents are currently limited to a
    /// string because of FLTK's limited ability to deal with the XDnD
    /// protocol. On the Mac, filename's etc may appear as contents.
    ///

    class DragDropEvent : public PointerEvent
    {
    public:
        enum Type
        {
            Enter,
            Leave,
            Move,
            Release
        };

        enum ContentType
        {
            File,
            URL,
            Text
        };

        // DragDropEvent() : PointerEvent() {}

        DragDropEvent(const std::string& name, const EventNode* sender,
                      Type type, ContentType ctype, const std::string& content,
                      unsigned int modifiers, int x, int y, /// position
                      int w, int h,                         /// size of domain
                      int ox = -1, int oy = -1,
                      void* data = 0) /// "push" values if any
            : PointerEvent(name, sender, modifiers, x, y, w, h, ox, oy, 0, data)
            , m_type(type)
            , m_contentType(ctype)
            , m_stringContent(content)
        {
        }

        DragDropEvent(const std::string& name, const EventNode* sender,
                      Type type, ContentType ctype, unsigned int modifiers,
                      int x, int y, /// position
                      int w, int h, /// size of domain
                      int ox = -1, int oy = -1,
                      void* data = 0) /// "push" values if any
            : PointerEvent(name, sender, modifiers, x, y, w, h, ox, oy, 0, data)
            , m_type(type)
            , m_contentType(ctype)
        {
        }

        Type type() const { return m_type; }

        ContentType contentType() const { return m_contentType; }

        void setStringContent(const std::string& s) { m_stringContent = s; }

        const std::string& stringContent() const { return m_stringContent; }

    private:
        Type m_type;
        ContentType m_contentType;
        std::string m_stringContent;
    };

    //----------------------------------------------------------------------
    ///
    /// GenericStringEvent
    ///
    /// A way to send a single string as an event. This event can also have a
    /// return string value.
    ///
    /// The stringContentVector() was added later to add additional
    /// information without causing existing code to break.  If you set
    /// the contents vector, the first argument should be the same value
    /// as the stringContent().
    ///

    class GenericStringEvent : public Event
    {
    public:
        // GenericStringEvent() : Event() {}

        GenericStringEvent(const std::string& name, const EventNode* sender,
                           const std::string& content, void* data = 0)
            : Event(name, sender, data)
            , m_stringContent(content)
        {
        }

        GenericStringEvent(const std::string& name, const EventNode* sender,
                           const std::string& content,
                           const std::string& senderName, void* data = 0)
            : Event(name, sender, data)
            , m_stringContent(content)
            , m_senderName(senderName)
        {
        }

        void setStringContent(const std::string& s) { m_stringContent = s; }

        const std::string& stringContent() const { return m_stringContent; }

        void setStringContentVector(const StringVector& sv)
        {
            m_stringContentVector = sv;
        }

        const StringVector& stringContentVector() const
        {
            return m_stringContentVector;
        }

        void setReturnContent(const std::string& s) const
        {
            m_returnContent = s;
        }

        const std::string& returnContent() const { return m_returnContent; }

        void setSenderName(const std::string& s) { m_senderName = s; }

        const std::string& senderName() const { return m_senderName; }

    private:
        std::string m_stringContent;
        std::string m_senderName;
        StringVector m_stringContentVector;
        mutable std::string m_returnContent;
    };

    ///----------------------------------------------------------------------
    ///
    /// PixelBlockTransferEvent
    ///
    /// Sends a block of pixels (which may be a whole image). Information
    /// about the rest of the image (its actual size, channels, etc)
    /// should already be known by the receiver.
    ///

    class PixelBlockTransferEvent : public Event
    {
    public:
        // PixelBlockTransferEvent() : Event() {}

        PixelBlockTransferEvent(const std::string& name,
                                const EventNode* sender,
                                const std::string& media,
                                const std::string& layer,
                                const std::string& view, int frame, int x,
                                int y, size_t w, size_t h, const void* pixels,
                                size_t size, void* data)
            : Event(name, sender, data)
            , m_media(media)
            , m_layer(layer)
            , m_view(view)
            , m_x(x)
            , m_y(y)
            , m_w(w)
            , m_h(h)
            , m_frame(frame)
            , m_pixels(pixels)
            , m_size(size)
        {
        }

        int x() const { return m_x; }

        int y() const { return m_y; }

        size_t width() const { return m_w; }

        size_t height() const { return m_h; }

        int frame() const { return m_frame; }

        const std::string& media() const { return m_media; }

        const std::string& layer() const { return m_layer; }

        const std::string& view() const { return m_view; }

        const void* pixels() const { return m_pixels; }

        size_t size() const { return m_size; }

    private:
        std::string m_media;
        std::string m_layer;
        std::string m_view;
        const void* m_pixels;
        size_t m_size;
        int m_x;
        int m_y;
        size_t m_w;
        size_t m_h;
        int m_frame;
    };

    //----------------------------------------------------------------------
    ///
    /// RawDataEvent
    ///
    /// An event holding
    ///

    class RawDataEvent : public Event
    {
    public:
        // RawDataEvent() : Event() {}

        RawDataEvent(const std::string& name, const EventNode* sender,
                     const std::string& contentType, const char* rawData,
                     size_t size, const char* utf8 = 0,
                     void* data = 0, // this is the base class "data"
                     const std::string& senderName = "")
            : Event(name, sender, data)
            , m_type(contentType)
            , m_rawData(rawData)
            , m_size(size)
            , m_utf8(utf8)
            , m_senderName(senderName)
        {
        }

        const char* rawData() const { return m_rawData; }

        size_t rawDataSize() const { return m_size; }

        const char* utf8() const { return m_utf8; }

        const std::string& contentType() const { return m_type; }

        void setSenderName(const std::string& s) { m_senderName = s; }

        const std::string& senderName() const { return m_senderName; }

        void setReturnContent(const std::string& s) const
        {
            m_returnContent = s;
        }

        const std::string& returnContent() const { return m_returnContent; }

    private:
        const char* m_utf8;
        const char* m_rawData;
        size_t m_size;
        std::string m_type;
        std::string m_senderName;
        mutable std::string m_returnContent;
    };

    ///----------------------------------------------------------------------
    ///
    /// Tablet Event
    ///

    class TabletEvent : public PointerEvent
    {
    public:
        enum TabletKind
        {
            UnknownTabletKind,
            PenKind,
            CursorKind,
            EraserKind
        };

        enum TabletDevice
        {
            NoTableDevice,
            PuckDevice,
            StylusDevice,
            AirBrushDevice,
            FourDMouseDevice,
            RotationStylusDevice
        };

        TabletEvent(const std::string& name, const EventNode* sender,
                    unsigned int modifiers, int x, int y, int w, int h, int ox,
                    int oy, unsigned int buttonStates, TabletKind kind,
                    TabletDevice device, double gx, double gy, double pres,
                    double tpres, double rot, int xtilt, int ytilt, int z = 0,
                    void* data = 0)
            : PointerEvent(name, sender, modifiers, x, y, w, h, ox, oy,
                           buttonStates, data)
            , _kind(kind)
            , _device(device)
            , _xFloat(gx)
            , _yFloat(gy)
            , _pressure(pres)
            , _tangentialPressure(tpres)
            , _rotation(rot)
            , _xTilt(xtilt)
            , _yTilt(ytilt)
            , _z(z)
        {
        }

        double gx() const { return _xFloat; }

        double gy() const { return _yFloat; }

        double pressure() const { return _pressure; }

        double tangentialPressure() const { return _tangentialPressure; }

        double rotation() const { return _rotation; }

        int xTilt() const { return _xTilt; }

        int yTilt() const { return _yTilt; }

        int z() const { return _z; }

    private:
        TabletKind _kind;
        TabletDevice _device;
        double _xFloat;
        double _yFloat;
        double _pressure;
        double _rotation;
        double _tangentialPressure;
        int _xTilt;
        int _yTilt;
        int _z;
    };

} // namespace TwkApp

#endif // __TwkApp__Event__h__
