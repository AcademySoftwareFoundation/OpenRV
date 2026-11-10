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
#include <IPCore/BrushCatalogue.h>
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

        if (const PixelBlockTransferEvent* pe = dynamic_cast<const PixelBlockTransferEvent*>(event->event))
        {
            const RvGraph::Sources& sources = s->rvgraph().imageSources();

            for (size_t i = 0; i < sources.size(); i++)
            {
                if (ImageSourceIPNode* node = dynamic_cast<ImageSourceIPNode*>(sources[i]))
                {
                    size_t index = node->mediaIndex(pe->media());

                    if (index != size_t(-1))
                    {
                        try
                        {
                            node->insertPixels(pe->view(), pe->layer(), pe->frame(), pe->x(), pe->y(), pe->width(), pe->height(),
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

    namespace
    {
        PyObject* brushCatalogue(PyObject* /*self*/, PyObject* /*args*/)
        {
            PyObject* brushes = PyList_New(0);

            if (brushes == nullptr)
            {
                return nullptr;
            }

            const Paint::BrushCatalogue& catalogue = Paint::bundledCatalogue();

            for (const Paint::BrushEntry& entry : catalogue.entries())
            {
                const Paint::BrushDefaults& brushDefaults = entry.params;

                PyObject* defaults =
                    Py_BuildValue("{s:d,s:d,s:d,s:d,s:d,s:O,s:d,s:d,s:d,s:d}", "spacingBias", brushDefaults.spacingBias, "spacingJitter",
                                  brushDefaults.spacingJitter, "opacityJitter", brushDefaults.opacityJitter, "radiusJitter",
                                  brushDefaults.radiusJitter, "rotationJitter", brushDefaults.rotationJitter, "rotateToStroke",
                                  brushDefaults.rotateToStroke ? Py_True : Py_False, "angle", brushDefaults.angle, "squish",
                                  brushDefaults.squish, "hardness", brushDefaults.hardness, "flow", brushDefaults.flow);

                if (defaults == nullptr)
                {
                    Py_DECREF(brushes);
                    return nullptr;
                }

                PyObject* brush = Py_BuildValue("{s:s,s:s,s:N}", "name", entry.name.c_str(), "tipPath",
                                                catalogue.tipPath(entry.tip).c_str(), "defaults", defaults);

                if (brush == nullptr)
                {
                    Py_DECREF(brushes);
                    return nullptr;
                }

                const int result = PyList_Append(brushes, brush);
                Py_DECREF(brush);

                if (result < 0)
                {
                    Py_DECREF(brushes);
                    return nullptr;
                }
            }

            return brushes;
        }
    } // namespace

    static PyMethodDef localmethods[] = {

        {"data", data, METH_NOARGS, "return session data object."},

        {"insertCreatePixelBlock", insertCreatePixelBlock, METH_VARARGS, "insert block of pixels into image source."},

        {"brushCatalogue", brushCatalogue, METH_NOARGS, "return the brush catalogue as a list of dicts."},

        {nullptr}};

    void* pyRvAppCommands() { return (void*)localmethods; }

} // namespace Rv
