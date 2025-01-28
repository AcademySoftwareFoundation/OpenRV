//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgImage_h_
#define _TwkImgImage_h_

#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Box.h>
#include <TwkMath/Color.h>
#include <algorithm>
#include <sys/types.h>
#include <string.h>

namespace TwkImg
{

    //******************************************************************************
    template <typename COLOR> class Image
    {
    public:
        typedef COLOR color_type;
        typedef size_t size_type;
        typedef COLOR* iterator;
        typedef const COLOR* const_iterator;
        typedef TwkMath::Vec2<size_t> vec_size_type;
        typedef int loc_type;
        typedef TwkMath::Vec2i vec_loc_type;
        typedef TwkMath::Box2i bounds_type;

        // Creates an image with zero size
        Image();

        // Create images with bounds
        // 0-(w-1), 0-(h-1)
        Image(size_type w, size_type h, COLOR* pixels = 0);
        Image(const vec_size_type& sze);

        // Create images within the bounds
        Image(const vec_loc_type& orig, const vec_size_type& sze);
        Image(const bounds_type& bnds);

        // Copy and assignment
        Image(const Image<COLOR>& img);
        Image<COLOR>& operator=(const Image<COLOR>& img);

        ~Image();

        // Same as size()[0] and size()[1]
        size_type width() const { return m_size[0]; }

        size_type height() const { return m_size[1]; }

        // This is the amount that you have to add to a pointer
        // to move it up by one scanline. Same as width()
        size_type stride() const { return m_size[0]; }

        // Same as bounds().size()
        const vec_size_type& size() const { return m_size; }

        // Change the size of the image
        void setSize(size_type w, size_type h);
        void setSize(const vec_size_type& size);

        // This version of the bounds gives the RELATIVE bounds,
        // which are always equal to (0,0) - (w-1, h-1)
        const bounds_type& bounds() const { return m_bounds; }

        // Rectangle in 2d space that tells us both the size
        // of this rectangle of pixels and also where they lie
        // in space. This is the ABSOLUTE bounds
        const bounds_type& boundsAbs() const { return m_boundsAbs; }

        // Return the lower left hand corner.
        // This can't be modified - it's just for reference.
        // Exactly equal to bounds().min
        const vec_loc_type& origin() const { return m_boundsAbs.min; }

        // Change the lower left hand corner. This doesn't
        // change the size of the image, just shifts the bounds.
        void setOrigin(const vec_loc_type& orig);

        // Same as width() * height()
        size_type numPixels() const { return m_numPixels; }

        //**************************************************************************
        // DATA ACCESS
        //**************************************************************************
        // First with iterators
        iterator begin() { return m_pixels; }

        const_iterator begin() const { return m_pixels; }

        const_iterator end() { return m_pixels + m_numPixels; }

        // Return a pixel by reference
        // Coordinate space is relative (not abs)
        // Because the coordinate space is relative we index
        // via size_type rather than vec_loc_type.
        const COLOR& operator()(size_type i) const;
        COLOR& operator()(size_type i);
        const COLOR& operator()(size_type x, size_type y) const;
        COLOR& operator()(size_type x, size_type y);
        const COLOR& operator()(const vec_size_type& loc) const;
        COLOR& operator()(const vec_size_type& loc);

        // Two dimensional array accessors
        const COLOR* operator[](size_type y) const;
        COLOR* operator[](size_type y);

        // Return pixel by reference in relative coordinate space
        const COLOR& pixel(size_type i) const;
        COLOR& pixel(size_type i);
        const COLOR& pixel(size_type x, size_type y) const;
        COLOR& pixel(size_type x, size_type y);
        const COLOR& pixel(const vec_size_type& loc) const;
        COLOR& pixel(const vec_size_type& loc);

        // These operations return the pixels in ABSOLUTE
        // coordinates, rather than relative coordinates.
        const COLOR& pixelAbs(loc_type x, loc_type y) const;
        COLOR& pixelAbs(loc_type x, loc_type y);
        const COLOR& pixelAbs(const vec_loc_type& loc) const;
        COLOR& pixelAbs(const vec_loc_type& loc);

        // This returns all the pixels
        const COLOR* pixels() const;
        COLOR* pixels();

        //
        //  This is a hack. Tells the Image to release its ownership
        //  of the pixel data. The Image is no longer valid after
        //  this call.
        //

        void releasePixels() { m_pixels = 0; }

        //**************************************************************************
        // OPERATIONS THAT MODIFY THE IMAGE
        //**************************************************************************
        // Clear entire image to black
        void clear();
        // Clear rectangle of image to black
        // Rectangle here is in relative coordinates
        void clearRect(const bounds_type& rect);
        // Clear absolute rectangle of the image.
        void clearRectAbs(const bounds_type& rectAbs);

        // Fill entire image with color
        void fill(const COLOR& fillCol);
        // Fill a rectangle of the image with a color
        // Rectangle here is in relative coordinates
        void fillRect(const bounds_type& rect, const COLOR& fillCol);
        // Fill rectangle in absolute coordinates.
        void fillRectAbs(const bounds_type& rectAbs, const COLOR& fillCol);

        // Copy from one image to another.
        // This is for copying within one offset rectangle to another,
        // because for straight copying, the copy constructor can be used.
        // The coordinates used here are RELATIVE coordinates.
        // The tile, in relative coordinates, in THIS image that will be altered
        // is represented by dstTile.
        // The tile, in relative coordinates, in the SRC image that will be
        // copied from is ( srcOffset, srcOffset + dstTile.size() - 1 ).
        void copyRect(const Image<COLOR>& src, const vec_size_type& srcOffset,
                      const bounds_type& dstTile, bool black = true);

        // The coordinates used here are ABSOLUTE coordinates,
        // for both the source and the dst images.
        // The tile, in absolute coordinates, in THIS image that will be altered
        // is represented by dstTile.
        // The tile, in absolute coordinates, in the SRC image that will be
        // copied from is ( srcOffset, srcOffset + dstTile.size() - 1 ).
        void copyRectAbs(const Image<COLOR>& src, const vec_loc_type& srcOffset,
                         const bounds_type& dstTile, bool black = true);

        // Transpose an image. An image which is size WxH becomes size HxW
        // with every pixel that was at x,y being moved to y,x. The origin
        // of the image remains undisturbed.
        // So many image processing operations benefit from this capability
        // That I put it here - it involves no significant amount of work
        // and it doesn't have any mallocs.
        void transpose();

    protected:
        void initPixels(COLOR* pixels = 0);

        // This is the size of the image. It is exactly the
        // same as m_bounds.size();
        vec_size_type m_size;

        // These bounds are just (0,0) to (w-1,h-1)
        bounds_type m_bounds;

        // The bounds represent both the image size as well
        // as where the image lies in space.
        bounds_type m_boundsAbs;

        // The actual pixels.
        COLOR* m_pixels;
        size_type m_numPixels;
        bool m_mypixels;
    };

    //******************************************************************************
    // APPLY RECT FUNCTION HEADERS
    //******************************************************************************
    template <typename C, class PIXELFUNC>
    void applyRect(Image<C>& dst, const Image<C>& src,
                   const typename Image<C>::vec_size_type& srcOffset,
                   const typename Image<C>::bounds_type& uncheckedDstTile,
                   PIXELFUNC& pfunc);

    template <typename C, class PIXELFUNC>
    void applyRectAbs(Image<C>& dst, const Image<C>& src,
                      const typename Image<C>::vec_loc_type& srcOffset,
                      const typename Image<C>::bounds_type& uncheckedDstTile,
                      PIXELFUNC& pfunc);

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Image<unsigned char> Img1uc;
    typedef Image<unsigned short> Img1us;
    typedef Image<float> Img1f;

    typedef Image<TwkMath::Col3uc> Img3uc;
    typedef Image<TwkMath::Col3us> Img3us;
    typedef Image<TwkMath::Col3f> Img3f;

    typedef Image<TwkMath::Col4uc> Img4uc;
    typedef Image<TwkMath::Col4us> Img4us;
    typedef Image<TwkMath::Col4f> Img4f;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename C> void Image<C>::initPixels(C* pixels)
    {
        if (m_size[0] <= 0 || m_size[1] <= 0)
        {
            m_size[0] = 0;
            m_size[1] = 0;
            m_numPixels = 0;
            m_pixels = NULL;
            m_bounds.makeEmpty();
            m_boundsAbs.makeEmpty();
            m_mypixels = true;
        }
        else
        {
            m_numPixels = m_size[0] * m_size[1];
            m_pixels = pixels ? pixels : new C[m_numPixels];
            m_mypixels = false;
        }
    }

    //******************************************************************************
    template <typename C>
    Image<C>::Image()
        : m_size(0, 0)
        , m_pixels(NULL)
        , m_numPixels(0)
        , m_mypixels(true)
    {
        m_bounds.makeEmpty();
        m_boundsAbs.makeEmpty();
    }

    //******************************************************************************
    template <typename C>
    Image<C>::Image(typename Image<C>::size_type w,
                    typename Image<C>::size_type h, C* pixels)
        : m_size(w, h)
        , m_bounds(vec_loc_type(0), vec_loc_type(w - 1, h - 1))
        , m_boundsAbs(vec_loc_type(0), vec_loc_type(w - 1, h - 1))
        , m_pixels(NULL)
        , m_mypixels(true)
    {
        initPixels(pixels);
    }

    //******************************************************************************
    template <typename C>
    Image<C>::Image(const typename Image<C>::vec_size_type& sze)
        : m_size(sze)
        , m_bounds(vec_loc_type(0), vec_loc_type(sze[0] - 1, sze[1] - 1))
        , m_boundsAbs(vec_loc_type(0), vec_loc_type(sze[0] - 1, sze[1] - 1))
        , m_pixels(NULL)
        , m_mypixels(true)
    {
        initPixels();
    }

    //******************************************************************************
    template <typename C>
    Image<C>::Image(const typename Image<C>::vec_loc_type& orig,
                    const typename Image<C>::vec_size_type& sze)
        : m_size(sze)
        , m_bounds(vec_loc_type(0), vec_loc_type(sze[0] - 1, sze[1] - 1))
        , m_boundsAbs(orig, orig + vec_loc_type(sze[0] - 1, sze[1] - 1))
        , m_pixels(NULL)
        , m_mypixels(true)
    {
        initPixels();
    }

    //******************************************************************************
    template <typename C>
    Image<C>::Image(const typename Image<C>::bounds_type& bnds)
        : m_size(bnds.size())
        , m_bounds(Image<C>::vec_loc_type(0, 0),
                   Image<C>::vec_loc_type(bnds.size(0) - 1, bnds.size(1) - 1))
        , m_boundsAbs(bnds)
        , m_pixels(NULL)
        , m_mypixels(true)
    {
        initPixels();
    }

    //******************************************************************************
    // Copy constructor
    template <typename C>
    Image<C>::Image(const Image<C>& copy)
        : m_size(copy.m_size)
        , m_bounds(copy.m_bounds)
        , m_boundsAbs(copy.m_boundsAbs)
        , m_pixels(NULL)
        , m_mypixels(true)
    {
        assert(m_bounds.size() == m_size);
        assert(m_boundsAbs.size() == m_size);

        initPixels();
        memcpy((void*)m_pixels, (const void*)copy.m_pixels,
               m_numPixels * sizeof(C));
    }

    //******************************************************************************
    // Called for when existing images are assigned to each other.
    // Not something to use lightly!!!
    template <typename C> Image<C>& Image<C>::operator=(const Image<C>& copy)
    {
        if ((&copy) != this)
        {
            delete[] m_pixels;
            m_size = copy.m_size;
            m_bounds = copy.m_bounds;
            m_boundsAbs = copy.m_boundsAbs;

            assert(m_bounds.size() == m_size);
            assert(m_boundsAbs.size() == m_size);

            initPixels();
            memcpy((void*)m_pixels, (const void*)copy.m_pixels,
                   m_numPixels * sizeof(C));
        }
    }

    //******************************************************************************
    template <typename C> Image<C>::~Image()
    {
        if (m_pixels && m_mypixels)
        {
            delete[] m_pixels;
        }

        m_pixels = 0;
    }

    //******************************************************************************
    template <typename C>
    void Image<C>::setSize(typename Image<C>::size_type w,
                           typename Image<C>::size_type h)
    {
        m_size[0] = w;
        m_size[1] = h;

        m_bounds.min = vec_loc_type(0, 0);
        m_bounds.max = vec_loc_type(w - 1, h - 1);

        m_boundsAbs.max = m_boundsAbs.min + vec_loc_type(w - 1, h - 1);

        delete[] m_pixels;
        initPixels();
        memset((void*)m_pixels, 0, m_numPixels * sizeof(C));
    }

    //******************************************************************************
    template <typename C>
    inline void Image<C>::setSize(const typename Image<C>::vec_size_type& sze)
    {
        setSize(sze[0], sze[1]);
    }

    //******************************************************************************
    template <typename C>
    inline void Image<C>::setOrigin(const typename Image<C>::vec_loc_type& orig)
    {
        m_boundsAbs.min = orig;
        m_boundsAbs.max = orig + vec_loc_type(m_size[0] - 1, m_size[1] - 1);
    }

    //******************************************************************************
    //******************************************************************************
    // PIXEL ACCESS FUNCTIONS
    //******************************************************************************
    //******************************************************************************
    template <typename C>
    inline const C& Image<C>::operator()(typename Image<C>::size_type i) const
    {
        assert(i >= 0 && i < m_numPixels);
        return m_pixels[i];
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::operator()(typename Image<C>::size_type i)
    {
        assert(i >= 0 && i < m_numPixels);
        return m_pixels[i];
    }

    //******************************************************************************
    template <typename C>
    inline const C& Image<C>::operator()(typename Image<C>::size_type x,
                                         typename Image<C>::size_type y) const
    {
        assert(x >= 0 && x < m_size[0] && y >= 0 && y < m_size[1]);
        return m_pixels[(x + stride() * y)];
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::operator()(typename Image<C>::size_type x,
                                   typename Image<C>::size_type y)
    {
        assert(x >= 0 && x < m_size[0] && y >= 0 && y < m_size[1]);
        return m_pixels[(x + stride() * y)];
    }

    //******************************************************************************
    template <typename C>
    inline const C&
    Image<C>::operator()(const typename Image<C>::vec_size_type& loc) const
    {
        return operator()(loc[0], loc[1]);
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::operator()(const typename Image<C>::vec_size_type& loc)
    {
        return operator()(loc[0], loc[1]);
    }

    //******************************************************************************
    template <typename C>
    inline const C*
    Image<C>::operator[](const typename Image<C>::size_type y) const
    {
        assert(y >= 0 && y < m_size[1]);
        return m_pixels + (stride() * y);
    }

    //******************************************************************************
    template <typename C>
    inline C* Image<C>::operator[](const typename Image<C>::size_type y)
    {
        assert(y >= 0 && y < m_size[1]);
        return m_pixels + (stride() * y);
    }

    //******************************************************************************
    template <typename C>
    inline const C& Image<C>::pixel(typename Image<C>::size_type i) const
    {
        assert(i >= 0 && i < m_numPixels);
        return m_pixels[i];
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::pixel(typename Image<C>::size_type i)
    {
        assert(i >= 0 && i < m_numPixels);
        return m_pixels[i];
    }

    //******************************************************************************
    template <typename C>
    inline const C& Image<C>::pixel(typename Image<C>::size_type x,
                                    typename Image<C>::size_type y) const
    {
        assert(x >= 0 && x < m_size[0] && y >= 0 && y < m_size[1]);
        return m_pixels[(x + stride() * y)];
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::pixel(typename Image<C>::size_type x,
                              typename Image<C>::size_type y)
    {
        assert(x >= 0 && x < m_size[0] && y >= 0 && y < m_size[1]);
        return m_pixels[(x + stride() * y)];
    }

    //******************************************************************************
    template <typename C>
    inline const C&
    Image<C>::pixel(const typename Image<C>::vec_size_type& loc) const
    {
        assert(loc[0] >= 0 && loc[0] < m_size[0] && loc[1] >= 0
               && loc[1] < m_size[1]);
        return m_pixels[(loc[0] + (stride() * loc[1]))];
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::pixel(const typename Image<C>::vec_size_type& loc)
    {
        assert(loc[0] >= 0 && loc[0] < m_size[0] && loc[1] >= 0
               && loc[1] < m_size[1]);
        return m_pixels[(loc[0] + (stride() * loc[1]))];
    }

    //******************************************************************************
    template <typename C>
    inline const C& Image<C>::pixelAbs(typename Image<C>::loc_type x,
                                       typename Image<C>::loc_type y) const
    {
        return pixel(x - m_boundsAbs.min[0], y - m_boundsAbs.min[1]);
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::pixelAbs(typename Image<C>::loc_type x,
                                 typename Image<C>::loc_type y)
    {
        return pixel(x - m_boundsAbs.min[0], y - m_boundsAbs.min[1]);
    }

    //******************************************************************************
    template <typename C>
    inline const C&
    Image<C>::pixelAbs(const typename Image<C>::vec_loc_type& loc) const
    {
        return pixel(loc - m_boundsAbs.min);
    }

    //******************************************************************************
    template <typename C>
    inline C& Image<C>::pixelAbs(const typename Image<C>::vec_loc_type& loc)
    {
        return pixel(loc - m_boundsAbs.min);
    }

    //******************************************************************************
    template <typename C> inline const C* Image<C>::pixels() const
    {
        return m_pixels;
    }

    //******************************************************************************
    template <typename C> inline C* Image<C>::pixels() { return m_pixels; }

    //******************************************************************************
    //******************************************************************************
    // IMAGE MODIFICATION FUNCTIONS
    //******************************************************************************
    //******************************************************************************
    template <typename C> inline void Image<C>::clear()
    {
        memset((void*)m_pixels, 0, m_numPixels * sizeof(C));
    }

    //******************************************************************************
    template <typename C>
    void Image<C>::clearRect(const typename Image<C>::bounds_type& rect)
    {
        // Fix rect. Can't decide if I like this or not.
        bounds_type irect = TwkMath::intersection(m_bounds, rect);
        if (irect.isEmpty())
        {
            return;
        }

        // Get copy size.
        size_t copySize = irect.size(0) * sizeof(C);
        assert(copySize > 0);
        if (copySize <= 0)
        {
            return;
        }

        // Loop over rows, copying.
        C* start = &(pixel(irect.min));
        C* end = &(pixel(irect.min[0], irect.max[1]));
        for (C* row = start; row <= end; row += stride())
        {
            memset((void*)row, 0, copySize);
        }
    }

    //******************************************************************************
    template <typename C>
    inline void
    Image<C>::clearRectAbs(const typename Image<C>::bounds_type& rect)
    {
        clearRect(rect - m_boundsAbs.min);
    }

    //******************************************************************************
    template <typename C> inline void Image<C>::fill(const C& fillCol)
    {
        for (int i = 0; i < m_numPixels; ++i)
        {
            m_pixels[i] = fillCol;
        }
    }

    //******************************************************************************
    template <typename C>
    void Image<C>::fillRect(const typename Image<C>::bounds_type& rect,
                            const C& fillCol)
    {
        // Fix rect.
        bounds_type irect = TwkMath::intersection(m_bounds, rect);
        if (irect.isEmpty())
        {
            return;
        }

        // Get num columns.
        int numCols = irect.size(0);
        assert(numCols > 0);

        // Loop over rows, copying.
        C* start = &pixel(irect.min);
        C* end = start + (irect.size(1) * stride());
        for (C* row = start; row < end; row += stride())
        {
            C* rowEnd = row + numCols;
            for (C* ptr = row; ptr < rowEnd; ptr++)
            {
                *ptr = fillCol;
            }
        }
    }

    //******************************************************************************
    template <typename C>
    inline void
    Image<C>::fillRectAbs(const typename Image<C>::bounds_type& rect,
                          const C& fillCol)
    {
        fillRect(rect - m_boundsAbs.min, fillCol);
    }

    //******************************************************************************
    template <typename C>
    void Image<C>::copyRect(
        const Image<C>& src, const typename Image<C>::vec_size_type& srcOffset,
        const typename Image<C>::bounds_type& uncheckedDstTile, bool black)
    {
        // Fix dstTile
        bounds_type irect = TwkMath::intersection(m_bounds, uncheckedDstTile);
        if (irect.isEmpty())
        {
            return;
        }

        // Determine irect offset
        // This is the difference between the lower left corners
        // of the irect tile from the dstTile.
        vec_size_type irectOffset(irect.min - uncheckedDstTile.min);

        // Create srcTile
        bounds_type fullSrcTile;
        fullSrcTile.min = srcOffset + irectOffset;
        fullSrcTile.max = fullSrcTile.min + (irect.max - irect.min);

        // Calculate intersection of srcTile with its bounds
        bounds_type srcTile = TwkMath::intersection(src.bounds(), fullSrcTile);
        if (srcTile.isEmpty())
        {
            clearRect(irect);
            return;
        }

        // Transform this offset into the dstTile.
        bounds_type dstTile;
        dstTile.min = irect.min + srcTile.min - fullSrcTile.min;
        dstTile.max = dstTile.min + (srcTile.max - srcTile.min);

        // Since we're being tolerant, we must handle the areas
        // around the dstTile that aren't being copied, and clear them
        // ************ irect ************
        // *                             *
        // *           blackT            *
        // *-----------------------------*
        // * blackL |  dstTile  | blackR *
        // *        |           |        *
        // *-----------------------------*
        // *           blackB            *
        // *                             *
        // *******************************

        // Calculate "tab stops".
        const C* srcRow = &(src.pixel(srcTile.min));
        C* dstRow = &pixel(irect.min);
        C* dstTileStart = &pixel(irect.min.x, dstTile.min.y);
        C* blackTStart = &(pixel(irect.min.x, dstTile.max.y)) + stride();
        C* dstEnd = &(pixel(irect.min.x, irect.max.y)) + stride();

        size_type irectCopySize = irect.size(0) * sizeof(C);
        int blackLSize = dstTile.min.x - irect.min.x;
        int dstTileSize = dstTile.size(0);
        int blackRSize = irect.max.x - dstTile.max.x;

        if (black)
        {
            // Do blackB
            for (; dstRow < dstTileStart; dstRow += stride())
            {
                memset((void*)dstRow, 0, irectCopySize);
            }
        }

        // Do middle band
        for (; dstRow < blackTStart; dstRow += stride())
        {
            C* dstCol = dstRow;

            if (black)
            {
                // do blackL
                if (blackLSize > 0)
                {
                    memset((void*)dstCol, 0, blackLSize * sizeof(C));
                    dstCol += blackLSize;
                }
            }

            // do dstTile
            memcpy((void*)dstCol, (const void*)srcRow, dstTileSize * sizeof(C));
            dstCol += dstTileSize;
            srcRow += src.stride();

            if (black)
            {
                // do blackR
                if (blackRSize > 0)
                {
                    memset((void*)dstCol, 0, blackRSize * sizeof(C));
                }
            }
        }

        if (black)
        {
            // Do blackT
            for (; dstRow < dstEnd; dstRow += stride())
            {
                memset((void*)dstRow, 0, irectCopySize);
            }
        }
    }

    //******************************************************************************
    template <typename C>
    inline void Image<C>::copyRectAbs(
        const Image<C>& src, const typename Image<C>::vec_loc_type& srcOffset,
        const typename Image<C>::bounds_type& uncheckedDstTile, bool black)
    {
        copyRect(src, srcOffset - src.origin(),
                 uncheckedDstTile - m_boundsAbs.min, black);
    }

    //******************************************************************************
    template <typename C> inline void Image<C>::transpose()
    {
        // Number of pixels stays the same.
        // Flip them over
        for (int y = 0; y < m_size[1]; ++y)
        {
            for (int x = 0; x < m_size[0]; ++x)
            {
                int index0 = x + (m_size[0] * y);
                int index1 = y + (m_size[1] * x);
                std::swap(m_pixels[index0], m_pixels[index1]);
            }
        }

        // Switch the size
        std::swap(m_size[0], m_size[1]);

        // Fix the bounds.
        m_bounds.max[0] = m_size[0] - 1;
        m_bounds.max[1] = m_size[1] - 1;
        m_boundsAbs.max[0] = m_boundsAbs.min[0] + m_bounds.max[0];
        m_boundsAbs.max[1] = m_boundsAbs.min[1] + m_bounds.max[1];
    }

    //******************************************************************************
    template <typename C, class PIXELFUNC>
    void applyRect(Image<C>& dst, const Image<C>& src,
                   const typename Image<C>::vec_size_type& srcOffset,
                   const typename Image<C>::bounds_type& uncheckedDstTile,
                   PIXELFUNC& pfunc)
    {
        // Fix dstTile
        typename Image<C>::bounds_type irect =
            TwkMath::intersection(dst.bounds(), uncheckedDstTile);
        if (irect.isEmpty())
        {
            return;
        }

        // Determine irect offset
        // This is the difference between the lower left corners
        // of the irect tile from the dstTile.
        typename Image<C>::vec_size_type irectOffset(irect.min
                                                     - uncheckedDstTile.min);

        // Create srcTile
        typename Image<C>::bounds_type fullSrcTile;
        fullSrcTile.min = srcOffset + irectOffset;
        fullSrcTile.max = fullSrcTile.min + (irect.max - irect.min);

        // Calculate intersection of srcTile with its bounds
        typename Image<C>::bounds_type srcTile =
            TwkMath::intersection(src.bounds(), fullSrcTile);
        if (srcTile.isEmpty())
        {
            return;
        }

        // Transform this offset into the dstTile.
        typename Image<C>::bounds_type dstTile;
        dstTile.min = irect.min + srcTile.min - fullSrcTile.min;
        dstTile.max = dstTile.min + (srcTile.max - srcTile.min);

        // Since we're being tolerant, we must handle the areas
        // around the dstTile that aren't being copied, and clear them
        // ************ irect ************
        // *                             *
        // *           blackT            *
        // *-----------------------------*
        // * blackL |  dstTile  | blackR *
        // *        |           |        *
        // *-----------------------------*
        // *           blackB            *
        // *                             *
        // *******************************

        // Calculate "tab stops".
        const C* srcRow = &(src.pixel(srcTile.min));
        C* dstRow = &(dst.pixel(irect.min));
        C* dstTileStart = &(dst.pixel(irect.min.x, dstTile.min.y));
        C* blackTStart =
            &(dst.pixel(irect.min.x, dstTile.max.y)) + dst.stride();
        C* dstEnd = &(dst.pixel(irect.min.x, irect.max.y)) + dst.stride();

        typename Image<C>::size_type irectCopySize = irect.size(0) * sizeof(C);
        int blackLSize = dstTile.min.x - irect.min.x;
        int dstTileSize = dstTile.size(0);
        int blackRSize = irect.max.x - dstTile.max.x;

        // Do middle band
        for (dstRow = dstTileStart; dstRow < blackTStart;
             dstRow += dst.stride())
        {
            // do dstTile
            C* dp = dstRow + blackLSize;
            C* dpEnd = dp + dstTileSize;
            const C* sp = srcRow;
            for (; dp < dpEnd; dp++, sp++)
            {
                pfunc(*sp, *dp);
            }
            srcRow += src.stride();
        }
    }

    //******************************************************************************
    template <typename C, class PIXELFUNC>
    inline void
    applyRectAbs(Image<C>& dst, const Image<C>& src,
                 const typename Image<C>::vec_loc_type& srcOffset,
                 const typename Image<C>::bounds_type& uncheckedDstTile,
                 PIXELFUNC& pfunc)
    {
        applyRect(dst, src, srcOffset - src.origin(),
                  uncheckedDstTile - dst.origin(), pfunc);
    }

} // End namespace TwkImg

#endif
