//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <MuTwkApp/EventType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/Module.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Object.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/StringHashTable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <TwkApp/Event.h>
#include <TwkApp/EventTable.h>
#include <TwkApp/VideoDevice.h>
#include <iostream>
#include <sstream>

namespace TwkApp
{
    using namespace Mu;
    using namespace std;
    typedef StringType::String String;

    //----------------------------------------------------------------------

    EventType::EventInstance::EventInstance(const Class* c)
        : ClassInstance(c)
        , event(0)
        , document(0)
    {
    }

    //----------------------------------------------------------------------

    EventType::EventType(Context* c, Class* super)
        : Class(c, "Event", super)
    {
    }

    EventType::~EventType() {}

    Object* EventType::newObject() const { return new EventInstance(this); }

    void EventType::deleteObject(Object* obj) const
    {
        delete static_cast<EventType::EventInstance*>(obj);
    }

    void EventType::outputValue(ostream& o, const Value& value, bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void EventType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                         ValueOutputState& state) const
    {
        EventInstance* i = *reinterpret_cast<EventInstance**>(vp);

        if (i)
        {
            o << "<Event ";

            if (i->event)
                o << "\"" << *i->event << "\"";
            else
                o << "nil";

            o << ">";
        }
        else
        {
            o << "nil";
        }
    }

    void EventType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        string tname = "Event";
        string rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "Event&", this),

                      new Function(c, "Event", BaseFunctions::dereference, Cast,
                                   Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", EventType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        addSymbols(
            new Function(c, "pointer", EventType::pointer, None, Return,
                         "vector float[2]", Args, tn, End),

            new Function(c, "relativePointer", EventType::relativePointer, None,
                         Return, "vector float[2]", Args, tn, End),

            new Function(c, "reference", EventType::reference, None, Return,
                         "vector float[2]", Args, tn, End),

            new Function(c, "domain", EventType::domain, None, Return,
                         "vector float[2]", Args, tn, End),

            new Function(c, "subDomain", EventType::subDomain, None, Return,
                         "vector float[2]", Args, tn, End),

            new Function(c, "domainVerticalFlip", EventType::domainVerticalFlip,
                         None, Return, "bool", Args, tn, End),

            new Function(c, "buttons", EventType::buttons, None, Return, "int",
                         Args, tn, End),

            new Function(c, "modifiers", EventType::modifiers, None, Return,
                         "int", Args, tn, End),

            new Function(c, "key", EventType::key, None, Return, "int", Args,
                         tn, End),

            new Function(c, "name", EventType::name, None, Return, "string",
                         Args, tn, End),

            new Function(c, "contents", EventType::contents, None, Return,
                         "string", Args, tn, End),

            new Function(c, "contentsArray", EventType::contentsArray, None,
                         Return, "string[]", Args, tn, End),

            new Function(c, "returnContents", EventType::returnContents, None,
                         Return, "string", Args, tn, End),

            new Function(c, "dataContents", EventType::dataContents, None,
                         Return, "byte[]", Args, tn, End),

            new Function(c, "sender", EventType::sender, None, Return, "string",
                         Args, tn, End),

            new Function(c, "contentType", EventType::contentType, None, Return,
                         "int", Args, tn, End),

            new Function(c, "contentMimeType", EventType::contentMimeType, None,
                         Return, "string", Args, tn, End),

            new Function(c, "timeStamp", EventType::timeStamp, None, Return,
                         "float", Args, tn, End),

            new Function(c, "reject", EventType::reject, None, Return, "void",
                         Args, tn, End),

            new Function(c, "setReturnContent", EventType::setReturnContent,
                         None, Return, "void", Args, tn, "string", End),

            new Function(c, "pressure", EventType::pressure, None, Return,
                         "float", Args, tn, End),

            new Function(c, "tangentialPressure", EventType::tangentialPressure,
                         None, Return, "float", Args, tn, End),

            new Function(c, "rotation", EventType::rotation, None, Return,
                         "float", Args, tn, End),

            new Function(c, "xTilt", EventType::xTilt, None, Return, "int",
                         Args, tn, End),

            new Function(c, "yTilt", EventType::yTilt, None, Return, "int",
                         Args, tn, End),

            new Function(c, "activationTime", EventType::activationTime, None,
                         Return, "float", Args, tn, End),

            new SymbolicConstant(c, "None", "int", Value(int(0))),
            new SymbolicConstant(c, "Shift", "int", Value(int(1 << 0))),
            new SymbolicConstant(c, "Control", "int", Value(int(1 << 1))),
            new SymbolicConstant(c, "Alt", "int", Value(int(1 << 2))),
            new SymbolicConstant(c, "Meta", "int", Value(int(1 << 3))),
            new SymbolicConstant(c, "Super", "int", Value(int(1 << 4))),
            new SymbolicConstant(c, "CapLock", "int", Value(int(1 << 5))),
            new SymbolicConstant(c, "NumLock", "int", Value(int(1 << 6))),
            new SymbolicConstant(c, "ScrollLock", "int", Value(int(1 << 7))),

            new SymbolicConstant(c, "Button1", "int", Value(1 << 0)),
            new SymbolicConstant(c, "Button2", "int", Value(1 << 1)),
            new SymbolicConstant(c, "Button3", "int", Value(1 << 2)),

            new SymbolicConstant(c, "UnknownObject", "int", Value(-1)),
            new SymbolicConstant(c, "BadObject", "int", Value(0)),
            new SymbolicConstant(c, "FileObject", "int", Value(1)),
            new SymbolicConstant(c, "URLObject", "int", Value(2)),
            new SymbolicConstant(c, "TextObject", "int", Value(3)),

            EndArguments);
    }

    static void throwBadArgumentException(const Mu::Node& node,
                                          Mu::Thread& thread, std::string msg)
    {
        ostringstream str;
        const Mu::MuLangContext* context =
            static_cast<const Mu::MuLangContext*>(thread.context());
        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());
        str << "in " << node.symbol()->fullyQualifiedName() << ": " << msg;
        e->string() += str.str().c_str();
        thread.setException(e);
        throw BadArgumentException(thread, e);
    }

    NODE_IMPLEMENTATION(EventType::print, void)
    {
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);

        if (i)
        {
            i->type()->outputValue(cout, Value(i));
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }
    }

    NODE_IMPLEMENTATION(EventType::name, Pointer)
    {
        Process* p = NODE_THREAD.process();
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);
        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());

        if (i)
        {
            NODE_RETURN(stype->allocate(i->event->name()));
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
            NODE_RETURN(0);
        }
    }

    NODE_IMPLEMENTATION(EventType::contentType, int)
    {
        Process* p = NODE_THREAD.process();
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        int rval = 0;

        if (const DragDropEvent* de =
                dynamic_cast<const DragDropEvent*>(i->event))
        {
            switch (de->contentType())
            {
            case DragDropEvent::File:
                rval = 1;
                break;
            case DragDropEvent::URL:
                rval = 2;
                break;
            case DragDropEvent::Text:
                rval = 3;
                break;
            default:
                rval = -1;
                break;
            }
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(rval);
    }

    NODE_IMPLEMENTATION(EventType::contentMimeType, Pointer)
    {
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);
        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        String* s = 0;

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const RawDataEvent* e = dynamic_cast<const RawDataEvent*>(i->event))
        {
            s = stype->allocate(e->contentType());
        }
        else if (const GenericStringEvent* e =
                     dynamic_cast<const GenericStringEvent*>(i->event))
        {
            s = stype->allocate("text/plain");
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "mime type not applicable");
        }

        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(EventType::contents, Pointer)
    {
        Process* p = NODE_THREAD.process();
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());

        String* s = 0;

        if (const DragDropEvent* de =
                dynamic_cast<const DragDropEvent*>(i->event))
        {
            s = stype->allocate(de->stringContent());
        }
        else if (const GenericStringEvent* e =
                     dynamic_cast<const GenericStringEvent*>(i->event))
        {
            s = stype->allocate(e->stringContent());
        }
        else if (const RawDataEvent* e =
                     dynamic_cast<const RawDataEvent*>(i->event))
        {
            if (e->utf8())
            {
                s = stype->allocate(e->utf8());
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "event not text");
            }
        }
        else if (const RenderEvent* re =
                     dynamic_cast<const RenderEvent*>(i->event))
        {
            s = stype->allocate(re->stringContent());
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(EventType::contentsArray, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        const DynamicArrayType* atype =
            static_cast<const Mu::DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(atype, 1);

        if (const GenericStringEvent* e =
                dynamic_cast<const GenericStringEvent*>(i->event))
        {
            const Event::StringVector& v = e->stringContentVector();
            const size_t n = v.size();

            if (n == 0)
            {
                array->resize(1);
                array->element<StringType::String*>(0) =
                    c->stringType()->allocate(e->stringContent());
            }
            else
            {
                array->resize(n);

                for (size_t q = 0; q < n; q++)
                {
                    array->element<StringType::String*>(q) =
                        c->stringType()->allocate(v[q]);
                }
            }
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(EventType::returnContents, Pointer)
    {
        Process* p = NODE_THREAD.process();
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());

        String* s = 0;

        if (const GenericStringEvent* e =
                dynamic_cast<const GenericStringEvent*>(i->event))
        {
            s = stype->allocate(e->returnContent());
        }

        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(EventType::dataContents, Pointer)
    {
        EventInstance* inst = NODE_ARG_OBJECT(0, EventInstance);
        DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);

        if (!inst)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const RawDataEvent* e =
                dynamic_cast<const RawDataEvent*>(inst->event))
        {
            int sz = e->rawDataSize();
            array->resize(sz);
            for (int i = 0; i < sz; ++i)
                array->element<Mu::byte>(i) = e->rawData()[i];
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(EventType::interp, Pointer)
    {
        EventInstance* inst = NODE_ARG_OBJECT(0, EventInstance);
        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        String* s = 0;

        if (!inst)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const RawDataEvent* e =
                dynamic_cast<const RawDataEvent*>(inst->event))
        {
            s = stype->allocate(e->contentType());
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "interp not applicable");
        }

        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(EventType::sender, Pointer)
    {
        Process* p = NODE_THREAD.process();
        EventInstance* i = NODE_ARG_OBJECT(0, EventInstance);
        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());

        if (!i)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        String* s = 0;

        if (const GenericStringEvent* e =
                dynamic_cast<const GenericStringEvent*>(i->event))
        {
            s = stype->allocate(e->senderName());
        }
        else if (const RawDataEvent* e =
                     dynamic_cast<const RawDataEvent*>(i->event))
        {
            s = stype->allocate(e->senderName());
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(EventType::key, int)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        int k;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const KeyEvent* ke = dynamic_cast<const KeyEvent*>(e->event))
        {
            k = ke->key();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        NODE_RETURN(k);
    }

    NODE_IMPLEMENTATION(EventType::pointer, Vector2f)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            v[0] = te->gx();
            v[1] = te->gy();
        }
        else if (const PointerEvent* pe =
                     dynamic_cast<const PointerEvent*>(e->event))
        {
            v[0] = pe->x();
            v[1] = pe->y();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return v;
    }

    NODE_IMPLEMENTATION(EventType::relativePointer, Vector2f)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            v[0] = te->gx();
            v[1] = te->gy();

            if (te->eventTable())
            {
                v[0] -= te->eventTable()->bbox().min.x;
                v[1] -= te->eventTable()->bbox().min.y;
            }
        }
        else if (const PointerEvent* pe =
                     dynamic_cast<const PointerEvent*>(e->event))
        {
            v[0] = pe->x();
            v[1] = pe->y();

            if (pe->eventTable())
            {
                v[0] -= pe->eventTable()->bbox().min.x;
                v[1] -= pe->eventTable()->bbox().min.y;
            }
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return v;
    }

    NODE_IMPLEMENTATION(EventType::reference, Vector2f)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const PointerEvent* pe =
                dynamic_cast<const PointerEvent*>(e->event))
        {
            v[0] = pe->startX();
            v[1] = pe->startY();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return v;
    }

    NODE_IMPLEMENTATION(EventType::domain, Vector2f)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const PointerEvent* pe =
                dynamic_cast<const PointerEvent*>(e->event))
        {
            v[0] = pe->w();
            v[1] = pe->h();
        }
        else if (const RenderEvent* re =
                     dynamic_cast<const RenderEvent*>(e->event))
        {
            v[0] = re->w();
            v[1] = re->h();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return v;
    }

    NODE_IMPLEMENTATION(EventType::subDomain, Vector2f)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const PointerEvent* pe =
                dynamic_cast<const PointerEvent*>(e->event))
        {
            v[0] = pe->w();
            v[1] = pe->h();
        }
        else if (const RenderEvent* re =
                     dynamic_cast<const RenderEvent*>(e->event))
        {
            v[0] = re->w();
            v[1] = re->h();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        if (e->event->eventTable())
        {
            v[0] = e->event->eventTable()->bbox().size().x;
            v[1] = e->event->eventTable()->bbox().size().y;
        }

        return v;
    }

    NODE_IMPLEMENTATION(EventType::domainVerticalFlip, bool)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        Vector2f v;

        if (e)
        {
            if (const RenderEvent* re =
                    dynamic_cast<const RenderEvent*>(e->event))
            {
                if (re->device())
                {
                    NODE_RETURN(re->device()->capabilities()
                                & VideoDevice::FlippedImage);
                }
                else
                {
                    throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                              "RenderEvent has unset device");
                }
            }
            else
            {
                throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                          "event is not a RenderEvent");
            }
        }

        NODE_RETURN(false);
    }

    NODE_IMPLEMENTATION(EventType::buttons, int)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        int b;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const PointerEvent* pe =
                dynamic_cast<const PointerEvent*>(e->event))
        {
            b = pe->buttonStates();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return b;
    }

    NODE_IMPLEMENTATION(EventType::modifiers, int)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        int b;

        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const ModifierEvent* pe =
                dynamic_cast<const ModifierEvent*>(e->event))
        {
            b = pe->modifiers();
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }

        return b;
    }

    NODE_IMPLEMENTATION(EventType::timeStamp, float)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }
        NODE_RETURN(float(e->event->timeStamp()));
    }

    NODE_IMPLEMENTATION(EventType::reject, void)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }
        e->event->handled = false;
    }

    NODE_IMPLEMENTATION(EventType::setReturnContent, void)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        String* s = NODE_ARG_OBJECT(1, String);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const GenericStringEvent* se =
                dynamic_cast<const GenericStringEvent*>(e->event))
        {
            se->setReturnContent(s->c_str());
        }
        else if (const RawDataEvent* rde =
                     dynamic_cast<const RawDataEvent*>(e->event))
        {
            rde->setReturnContent(s->c_str());
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "bad event type");
        }
    }

    NODE_IMPLEMENTATION(EventType::pressure, float)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            NODE_RETURN(float(te->pressure()));
        }
        else
        {
            NODE_RETURN(1.0f);
        }
    }

    NODE_IMPLEMENTATION(EventType::tangentialPressure, float)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            NODE_RETURN(float(te->tangentialPressure()));
        }
        else
        {
            NODE_RETURN(1.0f);
        }
    }

    NODE_IMPLEMENTATION(EventType::rotation, float)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            NODE_RETURN(float(te->rotation()));
        }
        else
        {
            NODE_RETURN(0.0f);
        }
    }

    NODE_IMPLEMENTATION(EventType::xTilt, int)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            NODE_RETURN(int(te->xTilt()));
        }
        else
        {
            NODE_RETURN(int(0));
        }
    }

    NODE_IMPLEMENTATION(EventType::yTilt, int)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const TabletEvent* te = dynamic_cast<const TabletEvent*>(e->event))
        {
            NODE_RETURN(int(te->yTilt()));
        }
        else
        {
            NODE_RETURN(int(0));
        }
    }

    NODE_IMPLEMENTATION(EventType::activationTime, float)
    {
        EventInstance* e = NODE_ARG_OBJECT(0, EventInstance);
        if (!e)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD, "nil argument");
        }

        if (const PointerButtonPressEvent* pe =
                dynamic_cast<const PointerButtonPressEvent*>(e->event))
        {
            NODE_RETURN(float(pe->activationTime()));
        }
        else
        {
            NODE_RETURN(0.0f);
        }
    }

} // namespace TwkApp
