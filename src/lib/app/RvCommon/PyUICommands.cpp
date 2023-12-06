//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <TwkGLF/GL.h>
#include <TwkGLF/GLVBO.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLState.h>
#endif
#endif

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <RvCommon/MediaFileTypes.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/RvDocument.h>
#include <RvCommon/RvFileDialog.h>
#include <RvCommon/RvNetworkDialog.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/RvWebManager.h>
#include <RvCommon/MuUICommands.h>
#include <IPCore/Session.h>
#include <MuPy/PyModule.h>
#include <PyTwkApp/PyEventType.h>
#include <PyTwkApp/PyInterface.h>
#include <QtCore/QtCore>
#include <QtWebEngineWidgets/QWebEnginePage>
#include <QtWidgets/QFileIconProvider>
#include <RvCommon/RvJavaScriptObject.h>
#include <QtGui/QtGui>
#include <RvApp/PyCommandsModule.h>
#include <TwkApp/Event.h>
#include <RvCommon/GLView.h> // WINDOWS NEEDS THIS LAST

namespace Rv {
using namespace std;
using namespace TwkApp;
using namespace IPCore;

static PyObject*
badArgument()
{
    PyErr_SetString(PyExc_Exception, "Bad argument");
    return NULL;
}


static QVariant::Type
pyTypeToQVariantType(PyObject* obj)
{
    if (PyBool_Check(obj)) return QVariant::Bool;
    else if (PyLong_Check(obj)) return QVariant::Int;
    else if (PyFloat_Check(obj)) return QVariant::Double;
    else if (PyBytes_Check(obj)) return QVariant::String;
    else if (PyUnicode_Check(obj)) return QVariant::String;
    else if (PyList_Check(obj)) return QVariant::List;
    return QVariant::List;
}

static PyObject*
qvariantToPyObject(QVariant::Type type, const QVariant& value)
{
    //
    //  As far as I can tell, all these functions "pass ownership" of a reference
    //  to the calling code, so we must INCREF the pyobject we return.
    //

    PyObject* ret = Py_None;

    switch (type)
    {
      case QVariant::Bool:
          ret = value.toBool() ? Py_True : Py_False;
          break;
      case QVariant::Int:
          ret = PyLong_FromLong(value.toInt());
          break;
      case QVariant::Double:
      case QMetaType::Float:    // does this happen?
          ret = PyFloat_FromDouble(value.toDouble());
          break;
      case QVariant::String:
          {
              //
              //  No you can't do this all in one step!  if you go all the way
              //  from qvar to const char*, the const char* often points at
              //  '\0' or garbage.
              //
              QByteArray ba = value.toString().toUtf8();
              const char* s = ba.constData();
              ret = PyUnicode_DecodeUTF8(s, strlen(s), "ignore");
          }
          break;
      case QVariant::StringList:
          {
              QStringList list = value.toStringList();
              PyObject* pylist = PyList_New(list.size());

              for (size_t i=0; i < list.size(); i++)
              {
                  QByteArray ba = list[i].toUtf8();
                  const char* s = ba.constData();
                  PyList_SetItem(pylist, i, PyUnicode_DecodeUTF8(s, strlen(s), "ignore"));
              }

              ret = pylist;
          }
          break;
      case QVariant::List:
          {
              QVariantList list = value.toList();
              PyObject* pylist = PyList_New(list.size());

              for (size_t i=0; i < list.size(); i++)
              {
                  PyList_SetItem(pylist, i, qvariantToPyObject(list[i].type(), list[i]));
              }

              ret = pylist;
          }
      default:
          break;
    }

    Py_XINCREF(ret);
    return ret;
}

static QVariant
pyObjectToQVariant(PyObject* pyobj)
{
    if (PyBool_Check(pyobj)) return QVariant(pyobj == Py_True ? true : false);
    else if (PyLong_Check(pyobj)) return QVariant(int(PyLong_AsLong(pyobj)));
    else if (PyFloat_Check(pyobj)) return QVariant(PyFloat_AsDouble(pyobj));
    else if (PyBytes_Check(pyobj)) return QVariant(QString(PyBytes_AsString(pyobj)));
    else if (PyUnicode_Check(pyobj))
    {
        PyObject* utf8 = PyUnicode_AsUTF8String(pyobj);
        QString qs = QString::fromUtf8(PyBytes_AsString(utf8));
        Py_XDECREF(utf8);
        return QVariant(qs);
    }
    else if (PyList_Check(pyobj))
    {
        QVariantList list;
        size_t n = PyList_Size(pyobj);

        for (size_t i = 0; i < n; i++)
        {
            list.append(pyObjectToQVariant(PyList_GetItem(pyobj, i)));
        }

        return QVariant(list);
    }

    return QVariant();
}

static PyObject *
readSettings(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    const char* group;
    const char* name;
    PyObject* defaultObj;

    if (!PyArg_ParseTuple(args, "ssO", &group, &name, &defaultObj))
    {
        return NULL;
    }

    if (!defaultObj || defaultObj == Py_None)
    {
        // THROW EXCEPTION
        PyErr_SetString(PyExc_Exception, "readSettings: default return value is None or undefined, but must match setting type");
        return NULL;
    }
    PyObject* ret = defaultObj;

    RV_QSETTINGS;
    settings.beginGroup(group);

    if (!settings.contains(name)) {
        Py_XINCREF(ret);
    } else {
        QVariant value = settings.value(name);
        QVariant::Type type = pyTypeToQVariantType(defaultObj);
        ret = qvariantToPyObject(type, value);
    }
    settings.endGroup();

    return ret;
}

static PyObject *
writeSettings(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    const char* group;
    const char* name;
    PyObject* value;

    if (!PyArg_ParseTuple(args, "ssO", &group, &name, &value))
    {
        return NULL;
    }
    if (!value) return NULL;

    RV_QSETTINGS;

    settings.beginGroup(group);
    settings.setValue(name, pyObjectToQVariant(value));
    settings.endGroup();

    Py_RETURN_NONE;
}

static void
popupMenuInternal (PyObject* pylist, QPoint& location)
{
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();

    s->receivingEvents(false);

    QPoint p = rvDoc->view()->mapToGlobal(location);

    if (pylist)
    {
        if (TwkApp::Menu* m = pyListToMenu("temp", pylist))
        {
            rvDoc->popupMenu(m, p);
        }
    }
    else
    {
        rvDoc->mainPopup()->popup(p);
    }
}

static PyObject *
popupMenu(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    PyEventObject* event;
    PyObject* pylist;

    if (!PyArg_ParseTuple(args, "O!O!", pyEventType(), &event, &PyList_Type, &pylist)) {
        return NULL;
    }

    QPoint lp;

    if (const TwkApp::PointerEvent* pevent =
        dynamic_cast<const TwkApp::PointerEvent*>(event->event))
    {
        lp = QPoint(pevent->x(), rvDoc->view()->height() - pevent->y() - 1);
    }
    else
    {
        lp = QPoint(0, rvDoc->view()->height() - 1);
    }

    popupMenuInternal (pylist, lp);

    Py_XINCREF(Py_None);
    return Py_None;
}

static PyObject *
popupMenuAtPoint(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    int x, y;
    PyObject* pylist;

    if (!PyArg_ParseTuple(args, "iiO!", &x, &y, &PyList_Type, &pylist))
    {
        return NULL;
    }

    QPoint lp (x, rvDoc->view()->height() - y - 1);

    popupMenuInternal (pylist, lp);

    Py_XINCREF(Py_None);
    return Py_None;
}


static PyObject *
sessionWindow(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();
    void *rvDoc = (void *)s->opaquePointer();

    PyObject* ret = Py_None;

    if (void *rvDoc = (void *)s->opaquePointer())
    {
        ret = PyLong_FromVoidPtr(rvDoc);
    }

    Py_XINCREF(ret);
    return ret;
}



static PyObject *
sessionGLView(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();

    PyObject* ret = Py_None;

    if (RvDocument *rvDoc = (RvDocument *)s->opaquePointer())
    {
        if (void *glview = (void *) rvDoc->view())
        {
            ret = PyLong_FromVoidPtr(glview);
        }
    }

    Py_XINCREF(ret);
    return ret;
}


static PyObject *
sessionTopToolBar(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();
    void *rvDoc = (void *)s->opaquePointer();

    PyObject* ret = Py_None;

    if (RvDocument *rvDoc = (RvDocument *)s->opaquePointer())
    {
        if (void *topViewToolBar = (void *) rvDoc->topViewToolBar())
        {
            ret = PyLong_FromVoidPtr(topViewToolBar);
        }
    }

    Py_XINCREF(ret);
    return ret;
}


static PyObject *
sessionBottomToolBar(PyObject *self, PyObject* args)
{
    PyLockObject locker;
    Session *s = Session::currentSession();
    PyObject* ret = Py_None;

    if (RvDocument *rvDoc = (RvDocument *)s->opaquePointer())
    {
        if (void *bottomViewToolBar = (void *) rvDoc->bottomViewToolBar())
        {
            ret = PyLong_FromVoidPtr(bottomViewToolBar);
        }
    }

    Py_XINCREF(ret);
    return ret;
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
static PyObject *
javascriptExport(PyObject *self, PyObject* args)
{
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());
    uint64_t       pointer;

    if (!PyArg_ParseTuple(args, "K", &pointer))
    {
        return NULL;
    }

    QWebFrame* webFramePointer = (QWebFrame*) pointer;
    RvJavaScriptObject* obj = new RvJavaScriptObject(doc, webFramePointer);

    Py_RETURN_NONE;
}
#else
static PyObject *
javascriptExport(PyObject *self, PyObject* args)
{
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());
    uint64_t       pointer;

    if (!PyArg_ParseTuple(args, "K", &pointer))
    {
        return NULL;
    }

    QWebEnginePage* webPagePointer = (QWebEnginePage*) pointer;
    RvJavaScriptObject* obj = new RvJavaScriptObject(doc, webPagePointer);

    Py_RETURN_NONE;
}

#endif

static PyMethodDef localmethods[] = {

    {"readSettings",
     readSettings,
     METH_VARARGS,
     ""},

    {"writeSettings",
     writeSettings,
     METH_VARARGS,
     ""},

    {"popupMenu",
     popupMenu,
     METH_VARARGS,
     ""},

    {"popupMenuAtPoint",
     popupMenuAtPoint,
     METH_VARARGS,
     ""},

    {"sessionWindow",
     sessionWindow,
     METH_NOARGS,
     ""},

    {"sessionGLView",
     sessionGLView,
     METH_NOARGS,
     ""},

    {"sessionTopToolBar",
     sessionTopToolBar,
     METH_NOARGS,
     ""},

    {"sessionBottomToolBar",
     sessionBottomToolBar,
     METH_NOARGS,
     ""},

    {"javascriptExport",
     javascriptExport,
     METH_VARARGS,
     ""},

    {NULL}
};

void*
pyUICommands()
{
    return (void*)localmethods;
}

} // Rv
