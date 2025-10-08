#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import rv.commands
import rv.rvtypes


def modeStatus(mode):
    return mode._active


def modeDrawOnEmpty(mode):
    return mode._drawOnEmpty


def modeDrawOnPresentation(mode):
    return mode._drawOnPresentation


def modeName(mode):
    return mode._modeName


def modeActive(mode):
    return mode._active


def modeActivate(mode):
    mode.activate()


def modeDeactivate(mode):
    mode.deactivate()


def modeRender(mode, event):
    mode.render(event)


def remotePyEval(event):
    "python eval() event contents"
    try:
        val = eval(event.contents())
        if val is not None:
            event.setReturnContent(str(val))
    except Exception:
        import sys

        e = sys.exc_info()
        event.setReturnContent("%s: %s" % (str(e[0]), str(e[1])))


def remotePyExec(event):
    "python exec() event contents"
    exec(event.contents())


def globalBindList(bindings):
    for b in bindings:
        (event, func, doc) = b
        rv.commands.bind("default", "global", event, func, doc)


def defineDefaultBindings():
    bindings = [
        ("remote-pyeval", remotePyEval, "Evaluate python expression returning result"),
        ("remote-pyexec", remotePyExec, "Execute python command (no return value)"),
    ]
    # , ("render", render, "Python render entry point")]
    globalBindList(bindings)


def newStateObject():
    s = rv.rvtypes.State()
    return s
