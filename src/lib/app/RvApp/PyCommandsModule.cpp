//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvApp/PyCommandsModule.h>

#include <RvApp/RvSession.h>

#include <Python.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPCore/Exception.h>
#include <PyTwkApp/PyEventType.h>
#include <TwkApp/Event.h>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

    static PyObject* badArgument()
    {
        PyErr_SetString(PyExc_Exception, "Bad argument");
        return NULL;
    }

    static PyObject* data(PyObject* self, PyObject* args)
    {
        RvSession* s = RvSession::currentRvSession();

        if (s->pyData())
        {
            Py_XINCREF(s->pyData());
            return (PyObject*)s->pyData();
        }
        else
        {
            Py_RETURN_NONE;
        }
    }

    static PyObject* insertCreatePixelBlock(PyObject* self, PyObject* args)
    {
        RvSession* s = RvSession::currentRvSession();
        PyEventObject* event;

        if (!PyArg_ParseTuple(args, "O!", TwkApp::pyEventType(), &event))
            return NULL;

        if (const PixelBlockTransferEvent* pe =
                dynamic_cast<const PixelBlockTransferEvent*>(event->event))
        {
            const RvGraph::Sources& sources = s->rvgraph().imageSources();

            for (size_t i = 0; i < sources.size(); i++)
            {
                if (ImageSourceIPNode* node =
                        dynamic_cast<ImageSourceIPNode*>(sources[i]))
                {
                    size_t index = node->mediaIndex(pe->media());

                    if (index != size_t(-1))
                    {
                        try
                        {
                            node->insertPixels(pe->view(), pe->layer(),
                                               pe->frame(), pe->x(), pe->y(),
                                               pe->width(), pe->height(),
                                               pe->pixels(), pe->size());
                        }
                        catch (PixelBlockSizeMismatchExc& exc)
                        {
                            ostringstream str;
                            str << "ERROR: " << exc.what() << " -- ";
                            str << "bad pixel block recieved";
                            PyErr_SetString(PyExc_Exception, str.str().c_str());
                            return NULL;
                        }
                    }
                }
            }
        }
        else
        {
            ostringstream str;
            str << "ERROR: bad event type";
            PyErr_SetString(PyExc_Exception, str.str().c_str());
            return NULL;
        }

        return Py_None;
    }

    static PyMethodDef localmethods[] = {

        {"data", data, METH_NOARGS, "return session data object."},

        {"insertCreatePixelBlock", insertCreatePixelBlock, METH_VARARGS,
         "insert block of pixels into image source."},

        {NULL}};

    void* pyRvAppCommands() { return (void*)localmethods; }

} // namespace Rv
