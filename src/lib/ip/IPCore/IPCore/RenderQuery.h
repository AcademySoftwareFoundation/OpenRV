//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__RenderQuery__h__
#define __IPCore__RenderQuery__h__
#include <TwkGLF/GLFBO.h>
#include <IPCore/IPImage.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLFence.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkMath/Box.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/IPNode.h>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{

    class RenderQuery
    {
    public:
        typedef TwkMath::Vec3f Vec3f;
        typedef ImageRenderer::RenderedImage RenderedImage;
        typedef ImageRenderer::GLTextureObject GLTextureObject;
        typedef IPImage::Matrix Matrix;
        typedef ImageRenderer::RenderedImagesVector RenderedImagesVector;
        typedef std::map<std::string, GLTextureObject> TaggedTextureImagesMap;
        typedef TwkApp::VideoDevice VideoDevice;

        // Constructor
        RenderQuery(const ImageRenderer* renderer)
            : m_renderer(renderer)
        {
        }

        //
        //  Query
        //
        void taggedTextureImages(TaggedTextureImagesMap&) const;

        void taggedWaveformImages(TaggedTextureImagesMap&) const;

        void renderedImages(RenderedImagesVector&) const;

        void imageCornersByHash(const IPImage::HashValue, std::vector<Vec3f>&,
                                bool stencil = true) const;

        void imageCornersForImage(const RenderedImage& image,
                                  std::vector<Vec3f>& points,
                                  bool stencil = true) const;

        //
        //  Find source near last rendered image x and y are in screen
        //  space coordinates [-1,1]. This will not return intern
        //

        void imagesByTag(RenderedImagesVector&,
                         const std::string& tag = "") const;

        //
        //  Find transforms by name. The name can be anything leading up
        //  to the fully qualified source or image name. So for example:
        //  if the rendered image is "source000.0/Track1/35" it would
        //  match "source000.0" or "source000.0/Track1" in addition to the
        //  full name.
        //

        void imageTransforms(const std::string& name, Matrix& modelMatrix,
                             Matrix& projectionMatrix, Matrix& textureMatrix,
                             Matrix& imageOrientation,
                             Matrix& imagePlacement) const;

        // Find frame-ratio by name. Name must match the exact source name.

        void imageFrameRatio(const std::string& name, double& frameRatio) const;

        //
        //  Given a source name, Return the corners of the image in
        //  order. (0,0), (1,0), (1,1), (0,1). See sourceTransforms
        //  comment regarding the full source name.
        //

        void imageCorners(const std::string& source, std::vector<Vec3f>& points,
                          bool stencil = true) const;

        void imageCorners(int index, std::vector<Vec3f>& points,
                          bool stencil = true) const;

        void imageCornersByTag(const std::string& name,
                               const std::string& value,
                               std::vector<Vec3f>& points,
                               bool stencil = true) const;

        // Deprecated
        void imageCornersByHash(const std::string& source,
                                std::vector<Vec3f>&) const;

    private:
        const ImageRenderer* m_renderer;
    };

} // namespace IPCore

#endif // __IPCore__RenderQuery__h__
