//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <IPCore/RenderQuery.h>
#include <stl_ext/string_algo.h>
#include <IPCore/IPNode.h>

#include <type_traits>

namespace IPCore {
    
using namespace std;
using namespace boost;

namespace {

// Returns true if all tokens are contained within source (delimited by '/') 
// and are in the same order
// Example: 
//    source = sourceGroup000000_source.0/track 1/1 
//    matches
//    tokens of sourceGroup000000_source.0/track 1 = [sourceGroup000000_source.0, track 1]
//    (The source in this example has an extra '1' token which corresponds to the frame number)
bool sourceHasMatchingTokens(const std::string& source,
                             const std::vector<string> & tokens)
{
    vector<string> stokens;
    stl_ext::tokenize(stokens, source, "/");

    const auto tokensSize = tokens.size();
    if ( stokens.size() < tokensSize ) return false;

    for (std::remove_const_t<decltype(tokensSize)> q = 0; q < tokensSize; ++q) { 
        if( stokens[q] != tokens[q] ) return false;
    }

    return true;
}

}
    
struct CompareRenderedImages
{
    CompareRenderedImages() {}

    bool operator() (const ImageRenderer::RenderedImage& a,
                     const ImageRenderer::RenderedImage& b)
    {
        return a.imageNum >= b.imageNum;
    }
};

void
RenderQuery::renderedImages(RenderedImagesVector& results) const
{
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();

    for (size_t i = 0, s = renderedImages->size(); i < s; ++i)
    {
        const RenderedImage& image = (*renderedImages)[i];

        if (image.device != m_renderer->currentDevice() ||
            (!image.render && !image.isVirtual) ||
            (!image.touched && !image.isVirtual)) continue;

        results.push_back(image);
    }

    //
    // we need to sort this because the rendered images order can be
    // different from the imageNum due to the fact sometimes the
    // renderer renders children in reversed order
    //

    sort (results.begin(), results.end(), CompareRenderedImages());
    
}

void
RenderQuery::imagesByTag(RenderedImagesVector& images,
                         const string& tag) const
{
    //
    //  return all images with matching tag
    //
    
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    for (size_t i = 0; i < renderedImages->size(); ++i)
    {
        const RenderedImage& image = (*renderedImages)[i];

        if (image.device != m_renderer->currentDevice() ||
            (tag != "" && image.tagMap.find(tag) == image.tagMap.end()))
        {
            continue;
        }

        if ((!image.render && !image.isVirtual) ||
            (!image.touched && !image.isVirtual) ||
            (image.isVirtual && image.node->group() &&
             image.node->group()->maximumNumberOfInputs() == 0))
        {
            continue;
        }

        images.push_back(image);
    }
    
    // we need to sort this because the rendered images order can be different
    // from the imageNum due to the fact sometimes the renderer renders children in reversed order
    sort (images.begin(), images.end(), CompareRenderedImages());
    
}

void
RenderQuery::imageTransforms(const string& name,
                             Matrix& M,
                             Matrix& P,
                             Matrix& T,
                             Matrix& O,
                             Matrix& Pl) const
{
    
    vector<string> tokens;
    stl_ext::tokenize(tokens, name, "/");
    
    //const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    RenderedImagesVector images;

    // for (size_t i = 0; i < renderedImages->size(); ++i)
    // {
    //     const RenderedImage& image = (*renderedImages)[i];
    //     if (image.device != m_renderer->currentDevice()) continue;
    //     if (!image.isVirtual && !image.touched) continue;
        
    //     images.push_back(image);
    // }
    
    //sort (images.begin(), images.end(), CompareRenderedImages());

    renderedImages(images);
    
    for (size_t i = 0; i < images.size(); ++i)
    {
        const RenderedImage& image = images[i];
        
        if ( (image.source == name) ||
             sourceHasMatchingTokens(image.source, tokens) )
        {
                M = image.globalMatrix;
                P = image.projectionMatrix;
                T = Matrix();
                O = image.orientationMatrix;
                Pl = image.placementMatrix;
                return;
        }
    }
    
    ostringstream str;
    str << "bad source name \"" << name << "\"";
    throw invalid_argument(str.str());
}

void RenderQuery::imageFrameRatio(const std::string& name,
                                  double& frameRatio) const
{
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    
    if ( renderedImages )
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, name, "/");

        for (size_t i = 0; i < renderedImages->size(); ++i)
        {
            const RenderedImage& image = (*renderedImages)[i];
            
            if (image.uncropHeight <= 0) continue;
            
            if ( (image.source == name) || 
                 sourceHasMatchingTokens(image.source, tokens) )
            {
                frameRatio =
                    static_cast<double>(image.uncropWidth) /
                    static_cast<double>(image.uncropHeight) * image.pixelAspect;
                return;
            }
        }
    }
    
    ostringstream str;
    str << "bad source name \"" << name << "\"";
    throw invalid_argument(str.str());
}


void
RenderQuery::imageCornersByHash(const IPImage::HashValue c,
                                vector<Vec3f>& points,
                                bool stencil) const
{
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    const VideoDevice* currentDevice = m_renderer->currentDevice();

    for (size_t i = 0, s = renderedImages->size(); i < s; ++i)
    {
        const RenderedImage& ri = (*renderedImages)[i];

        if (ri.index == c && ri.device == currentDevice)
        {
            imageCornersForImage(ri, points, stencil);
            return;
        }
    }

    RenderedImage image;
    imageCornersForImage(image, points, stencil);
}

void
RenderQuery::imageCornersByTag(const std::string& name,
                               const std::string& value,
                               vector<Vec3f>& points,
                               bool stencil) const
{
    RenderedImage image;
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();

    for (size_t j = 0; j < renderedImages->size(); ++j)
    {
        const RenderedImage& i = (*renderedImages)[j];
        IPImage::TagMap::const_iterator mi = i.tagMap.find(name);

        if (i.device == m_renderer->currentDevice() &&
            mi != i.tagMap.end() && mi->second == value)
        {
            imageCornersForImage(i, points, stencil);
            return;
        }
    }
    
    imageCornersForImage(image, points, stencil);
}

void
RenderQuery::imageCornersForImage(const RenderedImage& image,
                                  vector<Vec3f>& points,
                                  bool stencil) const
{
    const float iw   = image.width;
    const float ih   = image.height;
    const float uw   = image.uncropWidth;
    const float uh   = image.uncropHeight;
    const float ux   = image.uncropX;
    const float uy   = image.uncropY;

    const float wmin = ux / uw;
    const float hmin = (uh - uy - ih) / uh;
    const float wmax = (ux + iw) / uw;
    const float hmax = (uh - uy) / uh;

    float xmin = wmin + (wmax - wmin) * image.stencilBox.min.x;
    float ymin = hmin + (hmax - hmin) * image.stencilBox.min.y;
    float xmax = wmin + (wmax - wmin) * image.stencilBox.max.x;
    float ymax = hmin + (hmax - hmin) * image.stencilBox.max.y;
    
    if (!stencil)
    {
        xmin = image.imageBox.min.x;
        ymin = image.imageBox.min.y;
        xmax = image.imageBox.max.x;
        ymax = image.imageBox.max.y;
    }

    xmin *= image.pixelAspect * uw / uh;
    xmax *= image.pixelAspect * uw / uh;

    points.resize(4);
    const Matrix M = image.projectionMatrix * image.globalMatrix;
    points[0] = M * Vec3f(xmin, ymin, 0);
    points[1] = M * Vec3f(xmax, ymin, 0);
    points[2] = M * Vec3f(xmax, ymax, 0);
    points[3] = M * Vec3f(xmin, ymax, 0);
}

void
RenderQuery::imageCorners(int index, vector<Vec3f>& points, bool stencil) const
{
    return imageCornersByHash(index, points, stencil);
}

void
RenderQuery::imageCorners(const string& source, vector<Vec3f>& points, bool stencil) const
{
    vector<string> tokens;
    stl_ext::tokenize(tokens, source, "/");
    const VideoDevice* currentDevice = m_renderer->currentDevice();
    
    RenderedImagesVector images;
    renderedImages(images);
    
    for (size_t i = 0; i < images.size(); ++i)
    {
        const RenderedImage& image = images[i];
        
        if ((image.source == source) ||
            (image.node && image.node->name() == source) ||
            sourceHasMatchingTokens(image.source, tokens) )
        {
            imageCornersForImage(image, points, stencil);
            return;
        }
    }
    
    points.resize(0);
}

void
RenderQuery::taggedTextureImages(TaggedTextureImagesMap& results) const
{
    RenderedImage image;
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    const string tagName = IPImage::textureIDTagName();

    for (size_t i = 0, s = renderedImages->size(); i < s; ++i)
    {
        const RenderedImage& image = (*renderedImages)[i];

        IPImage::TagMap::const_iterator it2 = image.tagMap.find(tagName);

        if (it2 != image.tagMap.end())
        {
            if (image.textureID > 0)
            {
                results[(*it2).second] = GLTextureObject(image.textureID);
            }
        }
    }
}
    
void
RenderQuery::taggedWaveformImages(TaggedTextureImagesMap& results) const
{
    //
    // this function relies on the fact that output textures will be rendered first
    // before main controller display because renderExternal if (auxRender)
    // block calls this function, which happens at the end of display group render
    // if output textures are rendered after display group, these waveform images
    // wouldn't have been rendered when this function is called
    //
    
    RenderedImage image;
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();
    const string tagName = IPImage::textureIDTagName();
    const string waveformName = IPImage::waveformTagValue();

    for (size_t i = 0; i < renderedImages->size(); ++i)
    {
        const RenderedImage& image = (*renderedImages)[i];

        IPImage::TagMap::const_iterator it2 = image.tagMap.find(tagName);

        if (it2 != image.tagMap.end() && (*it2).second == waveformName)
        {
            results[(*it2).second] = GLTextureObject(image.textureID);
        }
    }
}
    
//
// DEPRECATED
//
void
RenderQuery::imageCornersByHash(const string& source,
                                vector<Vec3f>& points) const
{
    RenderedImage image;
    const RenderedImagesVector* renderedImages = m_renderer->renderedImages();

    for (size_t i = 0; i < renderedImages->size(); ++i)
    {
        const RenderedImage& image = (*renderedImages)[i];

        if (image.source == source)
        {
            imageCornersForImage(image, points);
            return;
        }
    }

    imageCornersForImage(image, points);
}
    
}
