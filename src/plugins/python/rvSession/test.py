#! /usr/bin/python
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#

import rvSession
import gto


def buildAnamorphicStuff(session):

    dpx = "/tweaklib/media/sequences/lions/vfx018_020_plate_v001.#.dpx"

    src = session.newNode("Source", "DPX Anamorphic Sequence")
    src.setMedia(dpx)

    src.setIgnoreChromaticities(True)
    src.setAspectRatio(2.0)

    return src


def buildColorStuff(session):

    exr = "/tweaklib/media/images/exr/Balls.exr"

    src = session.newNode("Source", "Exr Image")
    src.setMedia(exr)

    src.setExposure((3.0, 4.0, 5.0))

    return src


def buildRetimeStuff(session):

    video = "/tweaklib/media/movies/photojpg/upDialog.mov"

    # normal speed
    normal = session.newNode("Source", "Up Dialog")
    normal.setMedia(video)

    # retime to increase speed by 50%
    retimeUp = session.newNode("Retime", "Faster")
    retimeUp.addInput(normal)
    retimeUp.setVScale(0.75)

    # retime to decrease speed by 50%
    retimeDown = session.newNode("Retime", "Slower")
    retimeDown.addInput(normal)
    retimeDown.setVScale(1.5)

    # layout of all three
    layout = session.newNode("Layout", "Layout of Retimes")
    layout.setLayoutMode("row")
    layout.addInput(retimeUp)
    layout.addInput(normal)
    layout.addInput(retimeDown)

    return layout


def buildStereo(session):

    left = "/tweaklib/media/movies/photojpg/ct01.left.bs.elem.1k.gamma.mov"
    right = "/tweaklib/media/movies/photojpg/ct01.right.bs.elem.1k.gamma.mov"

    s1 = s.newNode("Source")
    s1.setUIName("stereoSource")
    s1.setMedia([left, right])

    return s1


def buildStackOfSeq(session):

    #    Make stack, turn on wipes
    stack = session.newNode("Stack", "Stack of Sequences")
    stack.setWipes(1)
    stack.setCompOp("replace")

    #    Make layout
    layout = session.newNode("Layout", "Layout of Sequences")
    layout.setLayoutMode("packed")

    #
    #    Make two sequences and connenct them as inputs to the stack
    #    and layout
    #

    seq1 = session.newNode("Sequence", "Sequence 1")
    stack.addInput(seq1)
    layout.addInput(seq1)

    seq2 = session.newNode("Sequence", "Sequence 2")
    stack.addInput(seq2)
    layout.addInput(seq2)

    #
    #   Media for the sources
    #

    audio = "/tweaklib/media/audio/woman_16-bit_44KHz_master.wav"
    video1 = "/tweaklib/media/movies/photojpg/hippoNumbered.mov"
    video2 = "/tweaklib/media/movies/photojpg/hippo.mov"

    #
    #   Make two sources, add them to the first sequence
    #   The order of the inputs is the order of the clips in the
    #   sequence view.
    #

    s1 = session.newNode("Source")
    s1.setUIName("201-248")
    s1.setMedia([video1, audio])
    s1.setCutIn(201)
    s1.setCutOut(248)
    s1.setAudioOffset(8.3333)
    seq1.addInput(s1)

    s2 = session.newNode("Source")
    s2.setUIName("249-298")
    s2.setMedia([video1, audio])
    s2.setCutIn(249)
    s2.setCutOut(298)
    s2.setAudioOffset(8.3333)
    seq1.addInput(s2)

    #
    #   Make two more sources, with more delayed audio, and add them
    #   to the second sequence.
    #

    s3 = session.newNode("Source")
    s3.setUIName("201-248 late audio")
    s3.setMedia([video2, audio])
    s3.setCutIn(201)
    s3.setCutOut(248)
    s3.setAudioOffset(8.5333)
    seq2.addInput(s3)

    s4 = session.newNode("Source")
    s4.setUIName("249-298 late audio")
    s4.setMedia([video2, audio])
    s4.setCutIn(249)
    s4.setCutOut(298)
    s4.setAudioOffset(8.5333)
    seq2.addInput(s4)

    return (stack, layout)


def addTextToSequence(session, arbitraryText="foo"):

    framerange = (1, 100)
    movie = "/tweaklib/media/movies/photojpg/hippoNumbered.mov"

    #
    #   Make Sequence
    #
    sequence = session.newNode("Sequence", "Text Sequence")

    s1 = session.newNode("Source")
    s1.setMedia(movie)

    #
    #   Also adding metadata
    #
    s1.setMetaData("here is the same text as metadata={0}".format(arbitraryText))

    #
    #   to make text last longer than one frame, you need to loop it over the desired range.
    #
    for i in range(int(framerange[0]), int(framerange[1]) + 1):
        textName = s1.setText(arbitraryText, 1, i)
        s1.setFrameNumberForText(i, textName)
        s1.setTextPosition(-0.65, -0.45)  # normalized-coordinates 0,0 is the center
        # Turn text red
        # s1.setTextColor (1,0,0)
        s1.setTextSize(0.01)
    sequence.addInput(s1)

    return sequence


def buildTransition(session, one, two):

    #
    #   Add transition node.
    #
    #   Wipe between "one" and "two" starting on frame 41, lasting for 20 frames
    #
    t = session.newNode("Wipe", "Wipe 1")
    t.setProperty("Wipe", "", "parameters", "startFrame", gto.FLOAT, 41.0)
    t.setProperty("Wipe", "", "parameters", "numFrames", gto.FLOAT, 20.0)

    t.addInput(one)
    t.addInput(two)

    return t


def buildOutputColor(session):

    #
    #   Add outputGroup color node.
    #
    o = session.setOutputGamma(2.2)

    return o


#######################################################################

#
#   Set this to whatever you want to view from the possibilities
#   below.  Note that all this stuff is in the file no matter what,
#   these options just setup the inital view (when the session file is
#   loaded).
#
view = "text"

s = rvSession.Session()
s.setFPS(24.0)

#
#   Anamorphic sample
#
anamorphicView = buildAnamorphicStuff(s)

#
#   Color settings sample
#
colorView = buildColorStuff(s)

#
#   Stereo sample
#
stereoView = buildStereo(s)

#
#   Stack of sequences and Layout of Sequences samples
#
(SoSview, LoSview) = buildStackOfSeq(s)

#
#   Some retime nodes
#
retimeView = buildRetimeStuff(s)

#
#   Some text addition nodes
#
sequenceTextView = addTextToSequence(s)

#
#   A Editorial Transition (wipe)
#
buildTransition(s, SoSview, LoSview)

#
#   A Output Color node for use by RVIO
#
buildOutputColor(s)

if view == "color":
    s.setDisplayLutName("/tweaklib/luts/csp/up1stop.csp")
    s.setViewNode(colorView)

if view == "stereo":
    s.setStereoType("anaglyph")
    s.setViewNode(stereoView)

elif view == "SoS":
    s.setViewNode(SoSview)

elif view == "LoS":
    s.setViewNode(LoSview)

elif view == "retime":
    s.setViewNode(retimeView)

elif view == "text":
    s.setViewNode(sequenceTextView)


s.write("sample.rv")
