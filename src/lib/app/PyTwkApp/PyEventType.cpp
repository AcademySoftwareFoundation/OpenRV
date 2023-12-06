//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <PyTwkApp/PyEventType.h>
#include <MuPy/PyModule.h>
#include <TwkApp/Event.h>
#include <TwkApp/EventTable.h>
#include <TwkApp/VideoDevice.h>

#include <TwkPython/PyLockObject.h>
#include <Python.h>

namespace TwkApp
{
  static PyObject* BadEvent = 0;

  static PyObject* badArgument()
  {
    PyErr_SetString( PyExc_Exception, "Bad argument" );
    return NULL;
  }

  static PyObject* buttons( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();

    if( const PointerEvent* pe =
            dynamic_cast<const PointerEvent*>( self->event ) )
    {
      return PyLong_FromLong( pe->buttonStates() );
    }

    return PyLong_FromLong( 0 );
  }

  static PyObject* modifiers( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    if( const ModifierEvent* pe =
            dynamic_cast<const ModifierEvent*>( self->event ) )
    {
      return PyLong_FromLong( pe->modifiers() );
    }

    return PyLong_FromLong( 0 );
  }

  static PyObject* pointer( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      return Py_BuildValue( "(f,f)", float( te->gx() ), float( te->gy() ) );
    }
    else if( const PointerEvent* pe =
                 dynamic_cast<const PointerEvent*>( self->event ) )
    {
      return Py_BuildValue( "(f,f)", float( pe->x() ), float( pe->y() ) );
    }
    else
    {
      return badArgument();
    }

    Py_RETURN_NONE;
  }

  static PyObject* relativePointer( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      float v0 = te->gx();
      float v1 = te->gy();

      if( te->eventTable() )
      {
        v0 -= te->eventTable()->bbox().min.x;
        v1 -= te->eventTable()->bbox().min.y;
      }

      return Py_BuildValue( "(f,f)", v0, v1 );
    }
    else if( const PointerEvent* pe =
                 dynamic_cast<const PointerEvent*>( self->event ) )
    {
      float v0 = pe->x();
      float v1 = pe->y();

      if( pe->eventTable() )
      {
        v0 -= pe->eventTable()->bbox().min.x;
        v1 -= pe->eventTable()->bbox().min.y;
      }

      return Py_BuildValue( "(f,f)", v0, v1 );
    }
    else
    {
      return badArgument();
    }

    Py_RETURN_NONE;
  }

  static PyObject* domain( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    float v[2];

    if( const PointerEvent* pe =
            dynamic_cast<const PointerEvent*>( self->event ) )
    {
      v[0] = pe->w();
      v[1] = pe->h();
    }
    else if( const RenderEvent* re =
                 dynamic_cast<const RenderEvent*>( self->event ) )
    {
      v[0] = re->w();
      v[1] = re->h();
    }
    else
    {
      return badArgument();
    }

    return Py_BuildValue( "(f,f)", v[0], v[1] );
  }

  static PyObject* domainVerticalFlip( PyEventObject* self, PyObject* args )
  {
    const Event* e = self->event;

    if( e )
    {
      if( const RenderEvent* re = dynamic_cast<const RenderEvent*>( e ) )
      {
        return re->device()->capabilities() & VideoDevice::FlippedImage
                   ? Py_True
                   : Py_False;
      }
      else
      {
        return badArgument();
      }
    }

    return Py_False;
  }

  static PyObject* subDomain( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    float v[2];

    if( const PointerEvent* pe =
            dynamic_cast<const PointerEvent*>( self->event ) )
    {
      v[0] = pe->w();
      v[1] = pe->h();
    }
    else if( const RenderEvent* re =
                 dynamic_cast<const RenderEvent*>( self->event ) )
    {
      v[0] = re->w();
      v[1] = re->h();
    }
    else
    {
      return badArgument();
    }

    if( self->event->eventTable() )
    {
      v[0] = self->event->eventTable()->bbox().size().x;
      v[1] = self->event->eventTable()->bbox().size().y;
    }

    return Py_BuildValue( "(f,f)", v[0], v[1] );
  }

  static PyObject* key( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    int k = -1;

    if( const KeyEvent* ke = dynamic_cast<const KeyEvent*>( self->event ) )
    {
      k = ke->key();
    }
    else
    {
      return badArgument();
    }

    return Py_BuildValue( "i", k );
  }

  static PyObject* name( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    return Py_BuildValue( "s", self->event->name().c_str() );
  }

  static PyObject* sender( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    const char* s = 0;

    if( const GenericStringEvent* e =
            dynamic_cast<const GenericStringEvent*>( self->event ) )
    {
      s = e->senderName().c_str();
    }
    else if( const RawDataEvent* e =
                 dynamic_cast<const RawDataEvent*>( self->event ) )
    {
      s = e->senderName().c_str();
    }
    else
    {
      return badArgument();
    }

    return Py_BuildValue( "s", s );
  }

  static PyObject* contents( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    const char* s = "";

    if( const DragDropEvent* de =
            dynamic_cast<const DragDropEvent*>( self->event ) )
    {
      s = de->stringContent().c_str();
    }
    else if( const GenericStringEvent* e =
                 dynamic_cast<const GenericStringEvent*>( self->event ) )
    {
      s = e->stringContent().c_str();
    }
    else if( const RawDataEvent* e =
                 dynamic_cast<const RawDataEvent*>( self->event ) )
    {
      if( e->utf8() )
      {
        s = e->utf8();
      }
      else
      {
        return badArgument();
      }
    }
    else if( const RenderEvent* re =
                 dynamic_cast<const RenderEvent*>( self->event ) )
    {
      s = re->stringContent().c_str();
    }
    else
    {
      return badArgument();
    }

    return Py_BuildValue( "s", s );
  }

  static PyObject* returnContents( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    const char* s = "";

    if( const GenericStringEvent* e =
            dynamic_cast<const GenericStringEvent*>( self->event ) )
    {
      s = e->returnContent().c_str();
    }

    return Py_BuildValue( "s", s );
  }

  static PyObject* dataContents( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    if( const RawDataEvent* e =
            dynamic_cast<const RawDataEvent*>( self->event ) )
    {
      int sz = e->rawDataSize();
      PyObject* list = PyList_New( sz );

      for( int i = 0; i < sz; ++i )
      {
        PyList_SetItem( list, i, PyLong_FromLong( e->rawData()[i] ) );
      }

      return list;
    }

    Py_RETURN_NONE;
  }

  static PyObject* contentType( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    int rval = 0;

    if( const DragDropEvent* de =
            dynamic_cast<const DragDropEvent*>( self->event ) )
    {
      switch( de->contentType() )
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
      return badArgument();
    }

    return PyLong_FromLong( rval );
  }

  static PyObject* contentMimeType( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    const char* s = "";

    if( const RawDataEvent* e =
            dynamic_cast<const RawDataEvent*>( self->event ) )
    {
      s = e->contentType().c_str();
    }
    else if( const GenericStringEvent* e =
                 dynamic_cast<const GenericStringEvent*>( self->event ) )
    {
      s = "text/plain";
    }
    else
    {
      return badArgument();
    }

    return PyBytes_FromString( s );
  }

  static PyObject* timeStamp( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    return PyFloat_FromDouble( self->event->timeStamp() );
  }

  static PyObject* reject( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    self->event->handled = false;
    Py_RETURN_NONE;
  }

  static PyObject* setReturnContent( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    const char* s;
    int ok = PyArg_ParseTuple( args, "s", &s ); /* A string */

    if( const GenericStringEvent* se =
            dynamic_cast<const GenericStringEvent*>( self->event ) )
    {
      se->setReturnContent( s );
    }
    else if( const RawDataEvent* rde =
                 dynamic_cast<const RawDataEvent*>( self->event ) )
    {
      rde->setReturnContent( s );
    }
    else
    {
      return badArgument();
    }

    Py_RETURN_NONE;
  }

  static PyObject* pressure( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    double d = 1.0;

    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      d = te->pressure();
    }

    return PyFloat_FromDouble( d );
  }

  static PyObject* tangentialPressure( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    double d = 1.0;

    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      d = te->tangentialPressure();
    }

    return PyFloat_FromDouble( d );
  }

  static PyObject* rotation( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    double d = 1.0;

    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      d = te->rotation();
    }

    return PyFloat_FromDouble( d );
  }

  static PyObject* xTilt( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    long d = 1.0;

    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      d = te->xTilt();
    }

    return PyLong_FromLong( d );
  }

  static PyObject* yTilt( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    long d = 1.0;

    if( const TabletEvent* te =
            dynamic_cast<const TabletEvent*>( self->event ) )
    {
      d = te->yTilt();
    }

    return PyLong_FromLong( d );
  }

  static PyObject* activationTime( PyEventObject* self, PyObject* args )
  {
    PyLockObject locker;
    if( !self->event ) return badArgument();
    double d = 0.0;

    if( const PointerButtonPressEvent* pe =
            dynamic_cast<const PointerButtonPressEvent*>( self->event ) )
    {
      d = pe->activationTime();
    }

    return PyFloat_FromDouble( d );
  }

  static PyMethodDef methods[] = {
      {"pointer", (PyCFunction)pointer, METH_NOARGS,
       "Return the pointer position"},
      {"relativePointer", (PyCFunction)relativePointer, METH_NOARGS,
       "Return the pointer position"},
      {"domain", (PyCFunction)domain, METH_NOARGS, "Return the domain size"},
      {"subDomain", (PyCFunction)subDomain, METH_NOARGS,
       "Return the domain size"},
      {"domainVerticalFlip", (PyCFunction)domainVerticalFlip, METH_NOARGS,
       "Return the domain vflip flag"},
      {"buttons", (PyCFunction)buttons, METH_NOARGS, "Return the button mask"},
      {"modifiers", (PyCFunction)modifiers, METH_NOARGS,
       "Return the modifier mask"},
      {"key", (PyCFunction)key, METH_NOARGS, "Return the key"},
      {"name", (PyCFunction)name, METH_NOARGS, "Return the key"},
      {"contents", (PyCFunction)contents, METH_NOARGS, "Return the key"},
      {"returnContents", (PyCFunction)returnContents, METH_NOARGS,
       "Return the key"},
      {"dataContents", (PyCFunction)dataContents, METH_NOARGS,
       "Return the key"},
      {"sender", (PyCFunction)sender, METH_NOARGS, "Return the key"},
      {"contentType", (PyCFunction)contentType, METH_NOARGS, "Return the key"},
      {"contentMimeType", (PyCFunction)contentMimeType, METH_NOARGS,
       "Return the key"},
      {"timeStamp", (PyCFunction)timeStamp, METH_NOARGS, "Return the key"},
      {"reject", (PyCFunction)reject, METH_NOARGS, "Return the key"},
      {"setReturnContent", (PyCFunction)setReturnContent, METH_VARARGS,
       "Return the key"},
      {"pressure", (PyCFunction)pressure, METH_NOARGS, "Return the key"},
      {"tangentialPressure", (PyCFunction)tangentialPressure, METH_NOARGS,
       "Return the key"},
      {"rotation", (PyCFunction)rotation, METH_NOARGS, "Return the key"},
      {"xTilt", (PyCFunction)xTilt, METH_NOARGS, "Return the key"},
      {"yTilt", (PyCFunction)yTilt, METH_NOARGS, "Return the key"},
      {"activationTime", (PyCFunction)activationTime, METH_NOARGS,
       "Return the key"},
      {NULL} /* Sentinel */
  };

static PyTypeObject type =
{
      PyVarObject_HEAD_INIT(NULL, 0)
      "Event",                 /*tp_name*/
      sizeof( PyEventObject ), /*tp_basicsize*/
      0,                       /* tp_itemsize */
      0,                       /* tp_dealloc */
      0,                       /* tp_print */
      0,                       /* tp_getattr */
      0,                       /* tp_setattr */
      0,                       /* tp_reserved */
      0,                       /* tp_repr */
      0,                       /* tp_as_number */
      0,                       /* tp_as_sequence */
      0,                       /* tp_as_mapping */
      0,                       /* tp_hash  */
      0,                       /* tp_call */
      0,                       /* tp_str */
      0,                       /* tp_getattro */
      0,                       /* tp_setattro */
      0,                       /* tp_as_buffer */
      Py_TPFLAGS_DEFAULT,      /*tp_flags*/
      "Application Event",     /* tp_doc */
      0,                       /* tp_traverse */
      0,                       /* tp_clear */
      0,                       /* tp_richcompare */
      0,                       /* tp_weaklistoffset */
      0,                       /* tp_iter */
      0,                       /* tp_iternext */
      methods,                 /* tp_methods */
  };

  PyTypeObject* pyEventType()
  {
    return &type;
  }

  PyObject* PyEventFromEvent( const Event* event, const Document* doc )
  {
    PyLockObject locker;
    PyTypeObject* etype = pyEventType();
    PyEventObject* e = (PyEventObject*) etype->tp_alloc(etype, 0);
    e->event = event;
    e->document = doc;
    return (PyObject*)e;
  }

  void initPyEventType()
  {
    //
  }

}  // namespace TwkApp
