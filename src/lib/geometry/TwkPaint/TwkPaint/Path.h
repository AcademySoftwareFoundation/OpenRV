//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkPaint__Path__h__
#define __TwkPaint__Path__h__
#include <TwkPaint/Paint.h>
#include <iostream>

namespace TwkPaint
{

    //
    //  Path
    //
    //  Describes a single "stroke" or continuous path. This path
    //  intentionally overlays double the "paint" when the path self
    //  intersects. In addition, this path includes texture coordinates
    //  intended for paint brush texture along the path or in the case of
    //  a radial texture in only the one dimension perpendicular to the
    //  path direction.
    //
    //  This class only builds the union geometry on adjacent segments
    //  when angle is less than 90deg. Otherwise it starts a new
    //  sub-path. (This is by design)
    //
    //  Is this really different than SVG?
    //

    class Path
    {
    public:
        enum PointType
        {
            BodyPoint,
            SplatPoint,
            StartPoint,
            EndPoint
        };

        struct Segment
        {
            Point points[4];
            Color colors[4];
            Vec vpn;
            float w0;
            bool none;
        };

        struct Join
        {
            Point points[3];
            Point center;
            Scalar width;
            Color colors[3];
            size_t indices[3];
            bool threePoints;
            bool positive;
            Point miterPoint;
            PointType type;
        };

        enum JoinStyle
        {
            NoJoin,
            BevelJoin,
            MiterJoin,
            RoundJoin
        };

        enum CapStyle
        {
            FlatCap,
            SquareCap,
            RoundCap
        };

        enum TextureStyle
        {
            NoTexture,
            RadiallySymmetric,
            VerticallySymmetric
        };

        enum Algorithm
        {
            FastAlgorithm,
            QualityAlgorithm
        };

        typedef std::vector<PointType> PointTypeVector;
        typedef std::vector<Segment> SegmentVector;
        typedef std::vector<Join> JoinVector;

        Path();
        ~Path();

        void clear();

        //
        //  Choose one of these to add points, do not mix them
        //

        void add(const Point& p);
        void add(const Point& p, Scalar width);
        void add(const Point& p, const Color& color);
        void add(const Point& p, Scalar width, const Color& color);

        //
        //  Width and/or color
        //

        bool isConstantWidth() const { return m_widths.empty(); }

        void setContantWidth(Scalar w)
        {
            m_widths.clear();
            m_width = w;
        }

        bool hasColor() const { return !m_colors.empty(); }

        void setNoColor() { m_colors.clear(); }

        //
        //  After computeGeometry you can get the output geometry. The smooth
        //  option will convert sparse input points to a smooth uniform input
        //  by creating bezier segments with c1 joins where needed based on the
        //  width.
        //

        void computeGeometry(JoinStyle, CapStyle, TextureStyle, Algorithm,
                             bool smooth = true, float smoothInterval = 1.0,
                             bool splatOnly = false);

        const PointArray& outputPoints() const { return m_rPoints; }

        const PointArray& outputTexCoords() const { return m_rTexCoords; }

        const IndexTriangleArray& outputTriangles() const { return m_rTris; }

        const ScalarArray& outputDirectionalities() const
        {
            return m_rDirectionCoords;
        }

        const PointArray& filteredPoints() const { return m_fpoints; }

        void dumpOBJ();

    protected:
        void smooth(float);
        void addStamp(const float dir, Point, float, Color, PointArray&,
                      PointArray&, IndexTriangleArray&, ScalarArray&,
                      ColorArray&);
        void filterPoints(PointArray&, ScalarArray&, ScalarArray&, ColorArray&,
                          PointTypeVector&, bool);
        void computeSegmentQuads(SegmentVector&, const PointArray& fpoints,
                                 const ScalarArray& fwidths,
                                 const ColorArray& fcolors,
                                 const PointTypeVector&, bool);

        void computeJoins(JoinVector& joins, const SegmentVector& segments,
                          const PointArray& fpoints, const ScalarArray& fwidths,
                          const ColorArray& fcolors,
                          const PointTypeVector& pointTypes, bool);

        void buildSegmentGeometry(const Segment& segment, Join& j0,
                                  const float dir0, Join& j1, const float dir1,
                                  PointArray& rPoints, PointArray& rTexCoords,
                                  IndexTriangleArray& rTris,
                                  ScalarArray& rDirectionCoords);

        void buildJoin(JoinStyle, const Join& j, const float dir,
                       PointArray& rPoints, PointArray& rTexCoords,
                       IndexTriangleArray& rTris,
                       ScalarArray& rDirectionCoords);

        void buildCap(CapStyle cs, const Join& j, const float directionality,
                      PointArray& rPoints, PointArray& rTexCoords,
                      IndexTriangleArray& rTris, ScalarArray& rDirectionCoords);

        void sampleCentripetalCatmull(const Point p0, const Point p1,
                                      const Point p2, const Point p3, size_t n,
                                      PointArray& newPoints);

    private:
        Scalar m_width;
        PointArray m_points;
        ScalarArray m_widths;
        ColorArray m_colors;
        PointArray m_fpoints;
        ScalarArray m_directionCoords; // stores the 'direction/location' of
                                       // current stroked point on the stroke
        PointArray m_rPoints;
        PointArray m_rTexCoords;
        ScalarArray m_rDirectionCoords; // stores the 'direction/location' of
                                        // current vertex on the stroke
        ColorArray m_rColors;
        IndexTriangleArray m_rTris;
        JoinStyle m_join;
        CapStyle m_cap;
        TextureStyle m_texture;
    };

} // namespace TwkPaint

#endif // __TwkPaint__Path__h__
