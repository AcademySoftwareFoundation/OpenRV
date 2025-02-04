//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieMuDraw__MovieMuDraw__h__
#define __MovieMuDraw__MovieMuDraw__h__
#include <Mu/MuProcess.h>
#include <Mu/Function.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/MovieReader.h>
#include <string>

namespace TwkMovie
{

    /// Draw crap on pased in FrameBuffers

    ///
    /// MovieMuDraw takes input from a single Movie and draws on top of it
    /// using FB controlled by a Mu module. The module must have a
    /// function called main which a specific signature. See the
    /// documentation of setFunction() for more information.
    ///

    class MovieMuDraw : public TwkMovie::Movie
    {
    public:
        //
        //  Types
        //

        typedef std::vector<std::string> StringVector;
        typedef Mu::STLVector<Mu::StringType::String*>::Type MuStringVector;
        typedef Mu::Function::ArgumentVector ArgumentVector;

        //
        //  Constructors
        //

        MovieMuDraw(Movie*, Mu::MuLangContext*, Mu::Process*,
                    TwkGLF::GLVideoDevice* d = 0);

        virtual ~MovieMuDraw();

        ///
        /// The module is loaded if necessary. Two function are searched for:
        /// main() and init(). Both are expected to have this signature in the
        /// module namespace. init() is called only once right before the first
        /// call to main(). init() is optional.
        ///
        ///      \: init (void; int w, int h,
        ///                     int tx, int ty,
        ///                     int tw, int th,
        ///                     bool stereo,
        ///                     bool rightEye,
        ///                     int frame,
        ///                     [string] argv);
        ///
        ///
        ///      \: main (void; int w, int h,
        ///                     int tx, int ty,
        ///                     int tw, int th,
        ///                     bool stereo,
        ///                     bool rightEye,
        ///                     int frame,
        ///                     [string] argv);
        ///
        ///         -or-
        ///
        ///      \: main (void; int w, int h,
        ///                     int tx, int ty,
        ///                     int tw, int th,
        ///                     bool stereo,
        ///                     bool rightEye,
        ///                     int frame,
        ///                     [string] argv,
        ///                     [(string,string)] keyvals);
        ///
        /// w and h parameters are the width and height of the overall
        /// framebuffer. tx, ty, tw, th, describe the region to be
        /// rendered. The actual size of the GL framebuffer will be tw,
        /// th. Currently these are the same as w and h and tx and ty are
        /// always 0, and 0.
        ///
        /// If stereo is true, and rightEye is false, the left eye is
        /// being rendered. Similarily if rightEye is true the rightEye is
        /// being rendered. If stereo is false, rightEye will alway be
        /// false.
        ///
        /// The frame parameter is the frame number used to retrieve the
        /// frame. This is often the "global" frame number.
        ///
        /// argv will contain any parameters passed in.
        ///
        /// If the second version of main with the keyvals arg is used,
        /// additional (keyword, value) tuples will be passed in to main.
        /// For example, "missing" will hold any missing frame events that
        /// occured. These are basically translated from the incoming fb's
        /// image attributes.
        ///

        void setFunction(const std::string& moduleName, const StringVector&);

        //
        //  Movie API
        //
        //  imageAtFrame() is special for a MovieMuDraw. The passed in
        //  FrameBuffer will determine the output frame buffer for GL. So
        //  passing in an 8 bit buffer will result in direct 8 bit output
        //  from GL.
        //

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);
        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
        virtual void flush();
        virtual Movie* clone() const;

        struct TT
        {
            Mu::Pointer name;
            Mu::Pointer value;
        };

    protected:
        void identifier(int frame, std::ostream&);

    private:
        Mu::MuLangContext* m_context;
        Mu::Process* m_process;
        Mu::Function* m_main;
        bool m_extraArg;
        Mu::Function* m_init;
        Movie* m_movie;
        StringVector m_argv;
        TwkGLF::GLVideoDevice* m_device;
    };

} // namespace TwkMovie

#endif // __MovieMuDraw__MovieMuDraw__h__
