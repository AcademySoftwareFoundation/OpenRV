//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkPaint/Path.h>
#include <TwkMath/Function.h>
#include <TwkMath/Math.h>
#include <TwkMath/Curve.h>
#include <TwkMath/LineAlgo.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Iostream.h>
#include <cmath>

namespace TwkPaint
{
    using namespace std;
    using namespace TwkMath;

    static const float pi = 3.14159265359f;

    Path::Path()
        : m_width(1.0)
    {
    }

    Path::~Path() {}

    void Path::clear()
    {
        m_points.clear();
        m_rPoints.clear();
        m_rTris.clear();
        m_rTexCoords.clear();
        m_widths.clear();
        m_directionCoords.clear();
        m_rDirectionCoords.clear();
    }

    void Path::add(const Point& p)
    {
        m_points.push_back(p);
        if (!m_widths.empty())
            m_widths.push_back(m_widths.back());
    }

    void Path::add(const Point& p, Scalar width)
    {
        m_points.push_back(p);
        m_widths.push_back(width);
    }

    void Path::add(const Point& p, const Color& color)
    {
        m_points.push_back(p);
        m_colors.push_back(color);
    }

    void Path::add(const Point& p, Scalar width, const Color& color)
    {
        m_points.push_back(p);
        m_widths.push_back(width);
        m_colors.push_back(color);
    }

    static void roundCap(size_t i0, size_t i1, size_t i2, const float dir,
                         PointArray& rPoints, PointArray& rTexCoords,
                         IndexTriangleArray& rTris,
                         ScalarArray& rDirectionCoords)
    {
        const Point& v0 = rPoints[i0];
        const Point& v1 = rPoints[i1];
        const Point& v2 = rPoints[i2];

        const Vec d0 = v2 - v0;
        const Vec d1 = v1 - v0;
        const float a = pi;
        const int div = a / (pi / 10.0f) + 0.5f;
        const float alpha = a / float(div);

        float b = angleBetween(d0, Vec(0, 1));

        Mat33f R, T, S;
        R.makeRotation(d0.x > 0 ? -b : b);
        T.makeTranslation(v0);
        S.makeScale(Vec(magnitude(d0)));
        Mat33f M = T * R * S;
        size_t n = rPoints.size();

        for (float q = 0; q < a; q += alpha)
        {

            if (q != 0.0)
            {
                Vec vt = Vec(sin(q), cos(q));
                rPoints.push_back(M * vt);
                rDirectionCoords.push_back(dir);
                rTexCoords.push_back(vt / 2.0f + 0.5f);
            }
        }

        size_t na = rPoints.size();

        IndexTriangle t(n, i2, i0);
        rTris.push_back(t);
        rDirectionCoords.push_back(dir);

        for (size_t q = n + 1; q < na; q++)
        {
            t[1] = t[0];
            t[0] = q;
            rTris.push_back(t);
        }

        t[1] = t[0];
        t[0] = i1;
        rTris.push_back(t);
    }

    void Path::addStamp(const float dir, Point point, float w, Color c,
                        PointArray& rPoints, PointArray& rTexCoords,
                        IndexTriangleArray& rTris,
                        ScalarArray& rDirectionCoords, ColorArray& rColors)
    {
        Point p0 = Point(point.x - w, point.y - w);
        Point p1 = Point(point.x + w, point.y - w);
        Point p2 = Point(point.x + w, point.y + w);
        Point p3 = Point(point.x - w, point.y + w);

        size_t n = rPoints.size();

        rPoints.push_back(p0);
        rTexCoords.push_back(Point(0, 0));
        rDirectionCoords.push_back(dir);
        rPoints.push_back(p1);
        rTexCoords.push_back(Point(1, 0));
        rDirectionCoords.push_back(dir);
        rPoints.push_back(p2);
        rTexCoords.push_back(Point(1, 1));
        rDirectionCoords.push_back(dir);
        rPoints.push_back(p3);
        rTexCoords.push_back(Point(0, 1));
        rDirectionCoords.push_back(dir);

        rTris.push_back(IndexTriangle(n, n + 1, n + 2));
        rTris.push_back(IndexTriangle(n, n + 2, n + 3));

        if (hasColor())
        {
            rColors.push_back(c);
            rColors.push_back(c);
            rColors.push_back(c);
            rColors.push_back(c);
        }
    }

    //
    // on top of filtering, this function also computes the 'location' of each
    // fpoint on the whole stroke. represented by m_directionCoords in this
    // example: f0
    //                   |
    //                   |
    //                  f1
    //                   \
//                    \f2
    //                     |
    //                     f3
    // f0 will have 'location' 0, and f3 will have 'location' 1, f1 will have
    // 'location' 2.0 / 5.0 f2 will have 'location' 4.0 / 5.0 this is a very
    // simple mechanism, further improvements might be needed
    //
    void Path::filterPoints(PointArray& fpoints, ScalarArray& directionCoords,
                            ScalarArray& fwidths, ColorArray& fcolors,
                            vector<PointType>& pointTypes, bool splatOnly)
    {
        const bool constWidth = isConstantWidth();
        const bool hasColor = this->hasColor();
        const Color black(0, 0, 0, 1);

        //
        //  Filter the points. We can't have two samples within width of
        //  each other. Right now we just omit close points as
        //  dups.
        //

        fpoints.push_back(m_points.front());
        m_directionCoords.push_back(0); // starting point has location 0
        pointTypes.push_back(splatOnly ? SplatPoint : StartPoint);
        if (!constWidth)
            fwidths.push_back(m_widths.front());
        if (hasColor)
            fcolors.push_back(m_colors.front());

        for (size_t i = 1; i < m_points.size(); i++)
        {
            Point p1 = m_points[i];
            Point p0 = fpoints.back();
            const float w1 = constWidth ? m_width : m_widths[i];
            const float w0 = constWidth ? m_width : fwidths.back();
            const Color c1 = hasColor ? m_colors[i] : black;
            const Color c0 = hasColor ? fcolors.back() : black;
            const float wm = max(w0, w1);
            const float mag = magnitude(p0 - p1);

            const char* sensitivityStr = getenv("TWK_PAINT_SPLAT_SENSITIVITY");
            const float sensitivity =
                (sensitivityStr) ? atof(sensitivityStr) : 1.0;

            if (splatOnly)
            {
                fpoints.push_back(p1);
                directionCoords.push_back(mag + m_directionCoords.back());
                pointTypes.push_back(SplatPoint);
                if (!constWidth)
                    fwidths.push_back(w1);
                if (hasColor)
                    fcolors.push_back(c1);
            }
            else
            {
                if (mag <= wm * sensitivity)
                {
                }
                else
                {
                    fpoints.push_back(p1);
                    directionCoords.push_back(mag + m_directionCoords.back());

                    const size_t s = fpoints.size();

                    if (s && pointTypes.back() == EndPoint)
                    {
                        pointTypes.push_back(StartPoint);
                    }
                    else
                    {
                        pointTypes.push_back(BodyPoint);
                    }

                    if (!constWidth)
                        fwidths.push_back(w1);
                    if (hasColor)
                        fcolors.push_back(c1);

                    if (s >= 3)
                    {
                        Point a = fpoints[s - 3];
                        Point b = fpoints[s - 2];
                        Point c = fpoints[s - 1];

                        if (pointTypes[s - 3] != SplatPoint
                            && pointTypes[s - 2] != SplatPoint
                            && pointTypes[s - 1] != SplatPoint
                            && angleBetween(a - b, c - b) < pi / 2.0)
                        {
                            // cout << "danger point at " << (s-2) << endl;
                            pointTypes[s - 2] = EndPoint;
                            fpoints[s - 1] = fpoints[s - 2];
                            directionCoords[s - 1] = m_directionCoords[s - 2];
                            pointTypes[s - 1] = StartPoint;
                            fpoints.push_back(p1);
                            directionCoords.push_back(
                                mag + m_directionCoords.back());
                            pointTypes.push_back(BodyPoint);

                            if (!constWidth)
                            {
                                fwidths[s - 1] = fwidths[s - 2];
                                fwidths.push_back(w1);
                            }

                            if (hasColor)
                            {
                                fcolors[s - 1] = fcolors[s - 2];
                                fcolors.push_back(c1);
                            }
                        }
                    }
                }
            }
        }

        // normalize all the direction coords to [0,1]
        float to = directionCoords.back();
        for (int i = 0; i < m_directionCoords.size(); ++i)
        {
            if (to > 0.0)
                directionCoords[i] /= to;
        }

        // fpoints.back() = m_points.back();
        if (pointTypes.back() == BodyPoint)
        {
            pointTypes.back() = EndPoint;
        }
        else if (pointTypes.back() == StartPoint)
        {
            pointTypes.back() = SplatPoint;
        }
    }

    void Path::computeSegmentQuads(SegmentVector& segments,
                                   const PointArray& fpoints,
                                   const ScalarArray& fwidths,
                                   const ColorArray& fcolors,
                                   const PointTypeVector& pointTypes,
                                   bool constWidth)
    {
        size_t n = fpoints.size();
        segments.resize(n - 1);

        bool usecolor = hasColor();

        //
        //  Compute segment quads
        //

        for (size_t i = 0; i < n - 1; i++)
        {
            Segment& s = segments[i];

            const PointType type0 = pointTypes[i];
            const PointType type1 = pointTypes[i + 1];

            if ((type0 == EndPoint && type1 == StartPoint)
                || type0 == SplatPoint || type1 == SplatPoint)
            {
                s.none = true;
                continue;
            }

            s.none = false;

            const Point p0 = fpoints[i];
            const Point p1 = fpoints[i + 1];
            const float w0 = constWidth ? m_width : fwidths[i];
            const float w1 = constWidth ? m_width : fwidths[i + 1];
            const Vec dir = normalize(p1 - p0);
            const Vec3 v0 = Vec3(dir.x, dir.y, 0);
            const Vec3 v1 = cross(v0, Vec3(0, 0, 1));
            const Vec vpn = normalize(Vec(v1.x, v1.y));

            assert(magnitude(vpn) <= 1.0001);

            s.points[0] = p0 - vpn * w0;
            s.points[1] = p0 + vpn * w0;
            s.points[2] = p1 + vpn * w1;
            s.points[3] = p1 - vpn * w1;

            s.vpn = vpn;
            s.w0 = w0;

            if (usecolor)
            {
                const Color& c0 = fcolors[i];
                const Color& c1 = fcolors[i + 1];
                s.colors[0] = c0;
                s.colors[1] = c0;
                s.colors[2] = c1;
                s.colors[3] = c1;
            }

#if 0
        if (i > 0)
        {
            Segment& s0 = segments[i-1];

            if (segmentsIntersect(s0.points[0], s0.points[1],
                                  s.points[2], s.points[3]))
            {
                //cout << "intersect seg " << i << " and " << (i-1) << endl;
            }
        }
#endif
        }
    }

    void Path::computeJoins(JoinVector& joins, const SegmentVector& segments,
                            const PointArray& fpoints,
                            const ScalarArray& fwidths,
                            const ColorArray& colors,
                            const PointTypeVector& pointTypes, bool constWidth)
    {
        joins.resize(segments.size() + 1);
        size_t n = joins.size();
        bool usecolor = hasColor();

        //
        //  Compute Join structs
        //

        for (size_t i = 0; i < n; i++)
        {
            Join& j = joins[i];
            j.type = pointTypes[i];
            j.center = fpoints[i];
            j.width = constWidth ? m_width : fwidths[i];

            if (j.type == SplatPoint)
            {
                if (usecolor)
                    j.colors[0] = colors[i];
            }
            else if (j.type == StartPoint)
            {
                //
                //  First point is always 2 vertices
                //

                const Segment& s0 = segments[i];

                j.points[0] = s0.points[0];
                j.points[1] = s0.points[1];
                j.threePoints = false;
                j.positive = true;

                if (usecolor)
                {
                    j.colors[0] = s0.colors[0];
                    j.colors[1] = s0.colors[1];
                }
            }
            else if (j.type == EndPoint)
            {
                //
                //  Last point has 2 vertices
                //

                const Segment& s0 = segments[i - 1];

                j.points[0] = s0.points[3];
                j.points[1] = s0.points[2];
                j.threePoints = false;
                j.positive = true;

                if (usecolor)
                {
                    j.colors[0] = s0.colors[3];
                    j.colors[1] = s0.colors[2];
                }
            }
            else if (j.type == BodyPoint)
            {
                //
                //  Mid Points could be 2 or 3 verts
                //

                const Segment& s1 = segments[i];
                const Segment& s0 = segments[i - 1];
                const Point& p0 = fpoints[i - 1];
                const Point& p1 = fpoints[i];
                const Point& p2 = fpoints[i + 1];
                const Vec v0 = normalize(p1 - p0);
                const Vec v1 = normalize(p2 - p1);
                const Vec3 c = cross(Vec3(v0.x, v0.y, 0), Vec3(v1.x, v1.y, 0));

                //
                //  Is this really a good idea? This is supposed to
                //  determine co-linearity (or not). It seems like the
                //  width, distance between points, etc all play a role
                //  here not just whether the points are co-linear. In
                //  addition, this test is basically arbitrary.
                //

                //
                // When a path has variable widths the segment edges
                // can become near collinear if the widths are just right.
                // This can causes the inner intersection point to
                // shoot off to infinity as the edges become more parallel,
                // creating ugly spikes. To avoid this, the intersection point
                // for both the positive and negative case is performed as if
                // path has a constant width.
                //

                if (std::abs(c.z) < Scalar(.01))
                {
                    //
                    //  o----o----o
                    //  |    |    |
                    //  +    +    +  <---- direction of stroke
                    //  |    |    |         + == stroke point
                    //  o----o----o         o == vertex
                    //

                    j.points[0] = s1.points[0];
                    j.points[1] = s1.points[1];
                    j.threePoints = false;
                    j.positive = true;

                    if (usecolor)
                    {
                        j.colors[0] = s1.colors[0];
                        j.colors[1] = s1.colors[2];
                    }
                }
                else if (c.z > 0)
                {
                    //       x
                    //
                    //     o + o     <-- direction of stroke
                    //    / \ / \
                //   /   o   \
                //  o   / \   o     POSITIVE CASE
                    //  |  /   \  |     + == stroke point
                    //  + /     \ +     o == vertex
                    //  |/       \|     x == other intersection
                    //  o         o

                    const float w = s1.w0;
                    const Vec2d a = Vec2d(fpoints[i - 1] - s0.vpn * w);
                    const Vec2d b = Vec2d(s0.points[3]);
                    const Vec2d c = Vec2d(s1.points[0]);
                    const Vec2d d = Vec2d(fpoints[i + 1] - s1.vpn * w);

                    Point ip = intersectionOfLines(a, b, c, d);

                    if (dot(ip - fpoints[i],
                            normalize(fpoints[i - 1] - fpoints[i]))
                        >= m_width)
                    {
                        // cout << "overlap at " << i << ", c.z = " << c.z <<
                        // endl;
                    }
                    else
                    {

                        j.miterPoint = intersectionOfLines(
                            Vec2d(s0.points[1]), Vec2d(s0.points[2]),
                            Vec2d(s1.points[1]), Vec2d(s1.points[2]));
                    }

                    j.points[0] = ip;
                    j.points[1] = s0.points[2];
                    j.points[2] = s1.points[1];
                    j.positive = true;
                    j.threePoints = true;

                    if (usecolor)
                    {
                        j.colors[0] = (s0.colors[2] + s1.colors[1]) / Scalar(2);
                        j.colors[1] = s0.colors[2];
                        j.colors[2] = s1.colors[1];
                    }
                }
                else // if (c.z < 0)
                {
                    //  o         o
                    //  |\       /|  <-- direction of stroke
                    //  + \     / +
                    //  |  \   /  |
                    //  o   \ /   o     NEGATIVE CASE
                    //   \   o   /      + == stroke point
                    //    \ / \ /       o == vertex
                    //     o + o        x == other intersection
                    //
                    //       x

                    const float w = s1.w0;
                    const Vec2d a = Vec2d(fpoints[i - 1] + s0.vpn * w);
                    const Vec2d b = Vec2d(s0.points[2]);
                    const Vec2d c = Vec2d(s1.points[1]);
                    const Vec2d d = Vec2d(fpoints[i + 1] + s1.vpn * w);

                    Point ip = intersectionOfLines(a, b, c, d);

                    if (dot(ip - fpoints[i],
                            normalize(fpoints[i + 1] - fpoints[i]))
                        >= m_width)
                    {
                        // cout << "overlap at " << i << ", c.z = " << c.z <<
                        // endl;
                    }
                    else
                    {
                        j.miterPoint = intersectionOfLines(
                            Vec2d(s0.points[0]), Vec2d(s0.points[3]),
                            Vec2d(s1.points[0]), Vec2d(s1.points[3]));
                    }

                    j.points[0] = ip;
                    j.points[1] = s1.points[0];
                    j.points[2] = s0.points[3];
                    j.positive = false;
                    j.threePoints = true;

                    if (usecolor)
                    {
                        j.colors[0] = (s1.colors[0] + s0.colors[3]) / Scalar(2);
                        j.colors[1] = s1.colors[0];
                        j.colors[2] = s0.colors[3];
                    }
                }
            }
        }
    }

    void Path::buildSegmentGeometry(const Segment& segment, Join& j0,
                                    const float dir0, Join& j1,
                                    const float dir1, PointArray& rPoints,
                                    PointArray& rTexCoords,
                                    IndexTriangleArray& rTris,
                                    ScalarArray& rDirectionCoords)
    {
        //
        //  First half
        //

        bool sharedSwap = false;
        bool swapSecond = false;

        float dir =
            0.5
            * (dir0 + dir1); // this is a naive scheme but might suffice for now

        if (j0.threePoints)
        {
            if (j0.positive)
            {
                // rPoints.push_back(j0.points[0]);
                rPoints.push_back(j0.points[2]);
                rDirectionCoords.push_back(dir0);
                rTexCoords.push_back(Point(.5, 1));
                // j0.indices[0] = rPoints.size() - 2;
                j0.indices[2] = rPoints.size() - 1;
            }
            else
            {
                rPoints.push_back(j0.points[1]);
                rDirectionCoords.push_back(dir0);
                rTexCoords.push_back(Point(.5, 0));
                // rPoints.push_back(j0.points[0]);
                j0.indices[1] = rPoints.size() - 1;
                // j0.indices[0] = rPoints.size() - 2;
                sharedSwap = true;
            }
        }
        // else if (i == 0)
        else if (j0.type == StartPoint)
        {
            rPoints.push_back(j0.center);
            rDirectionCoords.push_back(dir0);
            rTexCoords.push_back(Point(.5, .5));
            j0.indices[2] = rPoints.size() - 1;

            rPoints.push_back(j0.points[0]);
            rDirectionCoords.push_back(dir0);
            rPoints.push_back(j0.points[1]);
            rDirectionCoords.push_back(dir0);
            j0.indices[0] = rPoints.size() - 2;
            j0.indices[1] = rPoints.size() - 1;

            rTexCoords.push_back(Point(.5, 0));
            rTexCoords.push_back(Point(.5, 1));
        }
        else
        {
            swapSecond = true;
        }

        //
        //  Second half
        //

        if (j1.positive)
        {
            rPoints.push_back(j1.points[1]);
            rDirectionCoords.push_back(dir1);
            rPoints.push_back(j1.points[0]);
            rDirectionCoords.push_back(dir1);
            j1.indices[1] = rPoints.size() - 2;
            j1.indices[0] = rPoints.size() - 1;

            rTexCoords.push_back(Point(.5, 1));
            rTexCoords.push_back(Point(.5, 0));
        }
        else
        {
            assert(j1.threePoints);
            rPoints.push_back(j1.points[0]);
            rDirectionCoords.push_back(dir1);
            rPoints.push_back(j1.points[2]);
            rDirectionCoords.push_back(dir1);
            j1.indices[0] = rPoints.size() - 2;
            j1.indices[2] = rPoints.size() - 1;

            rTexCoords.push_back(Point(.5, 1));
            rTexCoords.push_back(Point(.5, 0));
        }

        //
        //  Triangles
        //

        size_t np = rPoints.size();

        if (j0.type == StartPoint)
        {
            rTris.push_back(IndexTriangle(np - 5, np - 1, np - 4));
            rTris.push_back(IndexTriangle(np - 5, np - 2, np - 1));
            rTris.push_back(IndexTriangle(np - 5, np - 3, np - 2));

            //
            //  This is a single short segment
            //

            if (j1.type == EndPoint)
            {
                rPoints.push_back(j1.center);
                rDirectionCoords.push_back(dir1);
                rTexCoords.push_back(Point(.5, .5));
                j1.indices[2] = rPoints.size() - 1;
            }
        }
        else if (j1.type == EndPoint)
        {
            if (sharedSwap)
            {
                rTris.push_back(IndexTriangle(np - 0, np - 1, np - 3));
                rTris.push_back(IndexTriangle(np - 0, np - 3, np - 5));
                rTris.push_back(IndexTriangle(np - 0, np - 5, np - 2));
            }
            else if (swapSecond)
            {
                rTris.push_back(IndexTriangle(np - 0, np - 1, np - 3));
                rTris.push_back(IndexTriangle(np - 0, np - 3, np - 4));
                rTris.push_back(IndexTriangle(np - 0, np - 4, np - 2));
            }
            else
            {
                rTris.push_back(IndexTriangle(np - 0, np - 1, np - 4));
                rTris.push_back(IndexTriangle(np - 0, np - 4, np - 3));
                rTris.push_back(IndexTriangle(np - 0, np - 3, np - 2));
            }

            rPoints.push_back(j1.center);
            rDirectionCoords.push_back(dir1);
            rTexCoords.push_back(Point(.5, .5));
            j1.indices[2] = rPoints.size() - 1;
        }
        else if (sharedSwap)
        {
            rTris.push_back(IndexTriangle(np - 5, np - 1, np - 3));
            rTris.push_back(IndexTriangle(np - 5, np - 2, np - 1));
        }
        else if (swapSecond)
        {
            rTris.push_back(IndexTriangle(np - 3, np - 4, np - 1));
            rTris.push_back(IndexTriangle(np - 4, np - 2, np - 1));
        }
        else
        {
            rTris.push_back(IndexTriangle(np - 3, np - 1, np - 4));
            rTris.push_back(IndexTriangle(np - 3, np - 2, np - 1));
        }
    }

    void Path::buildJoin(JoinStyle js, const Join& j, const float dir,
                         PointArray& rPoints, PointArray& rTexCoords,
                         IndexTriangleArray& rTris,
                         ScalarArray& rDirectionCoords)
    {
        if (j.type == SplatPoint)
        {
            addStamp(dir, j.center, j.width, j.colors[0], rPoints, rTexCoords,
                     rTris, rDirectionCoords, m_rColors);
            return;
        }

        switch (js)
        {
        default:
        case NoJoin:
            break;
        case BevelJoin:
            if (j.threePoints)
            {
                rTris.push_back(
                    IndexTriangle(j.indices[0], j.indices[1], j.indices[2]));
            }
            break;

        case MiterJoin:
        {
            m_rPoints.push_back(j.miterPoint);
            rDirectionCoords.push_back(dir);
            const size_t mindex = rPoints.size() - 1;
            m_rTexCoords.push_back(Vec(.5f, j.positive ? 1.0f : 0.0f));

            if (j.threePoints)
            {
                rTris.push_back(
                    IndexTriangle(j.indices[0], j.indices[1], mindex));

                rTris.push_back(
                    IndexTriangle(j.indices[0], mindex, j.indices[2]));
            }
        }
        break;

        case RoundJoin:
        {
            if (j.threePoints)
            {
                const size_t i0 = j.indices[0];
                const size_t i1 = j.indices[1];
                const size_t i2 = j.indices[2];
                const Point v0 = j.center;
                const Point v1 = rPoints[i1];
                const Point v2 = rPoints[i2];

                const Vec d0 = v2 - v0;
                const Vec d1 = v1 - v0;
                const float a = angleBetween(d0, d1);
                const int div = a / (pi / 10.0f) + 0.5f;
                const float alpha = a / float(div);

                float b = angleBetween(d0, Vec(0, 1));

                Mat33f R, T, S;
                R.makeRotation(d0.x > 0 ? -b : b);
                T.makeTranslation(v0);
                S.makeScale(Vec(magnitude(d0)));
                Mat33f M = T * R * S;
                size_t n = rPoints.size();

                for (float q = 0; q < a; q += alpha)
                {
                    if (q != 0.0)
                    {
                        rPoints.push_back(M * Vec(sin(q), cos(q)));
                        rDirectionCoords.push_back(dir);
                        rTexCoords.push_back(
                            Vec(.5f, j.positive ? 1.0f : 0.0f));
                    }
                }

                size_t na = rPoints.size();

                if (na == n)
                {
                    rTris.push_back(IndexTriangle(j.indices[0], j.indices[1],
                                                  j.indices[2]));
                }
                else
                {
                    IndexTriangle t(n, j.indices[2], j.indices[0]);
                    rTris.push_back(t);

                    for (size_t q = n + 1; q < na; q++)
                    {
                        t[1] = t[0];
                        t[0] = q;
                        rTris.push_back(t);
                    }

                    t[1] = t[0];
                    t[0] = j.indices[1];
                    rTris.push_back(t);
                }
            }
        }
        break;
        }
    }

    void Path::buildCap(CapStyle cs, const Join& j, const float directionality,
                        PointArray& rPoints, PointArray& rTexCoords,
                        IndexTriangleArray& rTris,
                        ScalarArray& rDirectionCoords)
    {
        switch (cs)
        {
        case FlatCap:
            break;
        case SquareCap:
        {
            bool start = j.type == StartPoint;

            const float dist = j.width;
            Vec dir = normalize(
                crossUp(rPoints[j.indices[0]] - rPoints[j.indices[1]]));
            if (j.type == StartPoint)
                dir = -dir;

            rPoints.push_back(Point(rPoints[j.indices[0]] + dir * dist));
            rDirectionCoords.push_back(directionality);
            rPoints.push_back(Point(rPoints[j.indices[1]] + dir * dist));
            rDirectionCoords.push_back(directionality);
            Scalar tx = start ? 0 : 1;
            rTexCoords.push_back(Point(tx, 0));
            rTexCoords.push_back(Point(tx, 1));

            // j.indices[2] is the tip point added above
            size_t n = rPoints.size();

            if (start)
            {
                rTris.push_back(
                    IndexTriangle(j.indices[2], j.indices[0], n - 2));
                rTris.push_back(
                    IndexTriangle(j.indices[1], j.indices[2], n - 1));
                rTris.push_back(IndexTriangle(n - 2, n - 1, j.indices[2]));
            }
            else
            {
                rTris.push_back(
                    IndexTriangle(j.indices[0], j.indices[2], n - 2));
                rTris.push_back(
                    IndexTriangle(j.indices[2], j.indices[1], n - 1));
                rTris.push_back(IndexTriangle(n - 1, n - 2, j.indices[2]));
            }
        }
        break;
        case RoundCap:
            if (j.type == StartPoint)
            {
                roundCap(j.indices[2], j.indices[0], j.indices[1],
                         directionality, rPoints, rTexCoords, rTris,
                         rDirectionCoords);
            }
            else if (j.type == EndPoint)
            {
                roundCap(j.indices[2], j.indices[1], j.indices[0],
                         directionality, rPoints, rTexCoords, rTris,
                         rDirectionCoords);
            }
            break;
        }
    }

    void Path::computeGeometry(JoinStyle js, CapStyle cs, TextureStyle ts,
                               Algorithm algo, bool dosmooth,
                               float smoothInterval, bool splatOnly)
    {
        if (dosmooth)
            smooth(smoothInterval);

        m_join = js;
        m_cap = cs;
        m_texture = ts;

        m_rPoints.clear();
        m_rTexCoords.clear();
        m_rTris.clear();
        m_rDirectionCoords.clear();

        const bool constWidth = isConstantWidth();
        const bool hasColor = this->hasColor();
        PointArray& points = m_rPoints;
        vector<PointType> pointTypes;
        PointArray& fpoints = m_fpoints;
        fpoints.clear();
        m_directionCoords.clear();
        ScalarArray fwidths;
        ColorArray fcolors;
        const Color black(0, 0, 0, 1);

        //
        //  Filter the points. We can't have two samples within width of
        //  each other. Right now we just omit close points as
        //  dups.
        //

        filterPoints(fpoints, m_directionCoords, fwidths, fcolors, pointTypes,
                     splatOnly);

        const size_t n = fpoints.size();

        if (n == 1)
        {
            //
            //  This the case for a single sample point
            //

            addStamp(0, fpoints.front(), constWidth ? m_width : fwidths.front(),
                     hasColor ? fcolors.front() : black, m_rPoints,
                     m_rTexCoords, m_rTris, m_rDirectionCoords, m_rColors);
            return;
        }

        //
        //  Build the base segment quads that overlap. These will eventually be
        //  trimmed into the final geometry.
        //

        vector<Segment> segments;
        computeSegmentQuads(segments, fpoints, fwidths, fcolors, pointTypes,
                            constWidth);

        //
        //  Compute the join struct for each point using the segment info
        //

        vector<Join> joins;
        computeJoins(joins, segments, fpoints, fwidths, fcolors, pointTypes,
                     constWidth);

        //
        //  Compute final segment triangles, share vertices whenever possible
        //

        assert(fpoints.size() == joins.size());
        for (size_t i = 0; i < n - 1; i++)
        {
            const Segment& s = segments[i];
            if (s.none)
                continue;

            const float diri = m_directionCoords[i];
            const float diriplus1 = m_directionCoords[i + 1];
            buildSegmentGeometry(s, joins[i], diri, joins[i + 1], diriplus1,
                                 m_rPoints, m_rTexCoords, m_rTris,
                                 m_rDirectionCoords);
        }

        //
        //  Build final triangles for each join
        //

        for (size_t i = 0; i < joins.size(); i++)
        {
            Join& j = joins[i];
            if (j.type == BodyPoint || j.type == SplatPoint)
            {
                buildJoin(js, j, m_directionCoords[i], m_rPoints, m_rTexCoords,
                          m_rTris, m_rDirectionCoords);
            }
        }

        //
        //  Finally, create final triangles for the caps
        //

        for (size_t i = 0; i < joins.size(); i++)
        {
            const Join& j = joins[i];
            if (j.type != StartPoint && j.type != EndPoint)
                continue;
            buildCap(cs, j, m_directionCoords[i], m_rPoints, m_rTexCoords,
                     m_rTris, m_rDirectionCoords);
        }

        assert(m_rPoints.size() == m_rDirectionCoords.size());

#if 1
        for (size_t i = 0; i < m_rTris.size(); i++)
        {
            const IndexTriangle& t = m_rTris[i];

            const Vec a2 = m_rPoints[t[0]];
            const Vec b2 = m_rPoints[t[1]];
            const Vec c2 = m_rPoints[t[2]];

            Vec3 a(a2.x, a2.y, 0);
            Vec3 b(b2.x, b2.y, 0);
            Vec3 c(c2.x, c2.y, 0);

            if (cross(b - a, c - a).z < 0)
            {
                // cout << "inverted at triangle " << i << endl;
            }
        }
#endif
    }

    void Path::dumpOBJ()
    {
        for (size_t i = 0; i < m_rPoints.size(); i++)
        {
            cout << "v " << m_rPoints[i].x << " " << m_rPoints[i].y << " 0"
                 << endl;
        }

        for (size_t i = 0; i < m_rTexCoords.size(); i++)
        {
            cout << "vt " << m_rTexCoords[i].x << " " << m_rTexCoords[i].y
                 << endl;
        }

        for (size_t i = 0; i < m_rTris.size(); i++)
        {
            IndexTriangle t = m_rTris[i];
            t += IndexTriangle(1, 1, 1);
            cout << "f " << t[0] << "/" << t[0] << " " << t[1] << "/" << t[1]
                 << " " << t[2] << "/" << t[2] << endl;
        }
    }

    void Path::sampleCentripetalCatmull(const Point p0, const Point p1,
                                        const Point p2, const Point p3,
                                        size_t n, PointArray& newPoints)
    {

        // using centripetal catmull interpolation
        // http://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

        Scalar t0, t1, t2, t3;
        Point a1, a2, a3, b1, b2;
        // float alpha = 0.5; alpha 0.5 is centripetal

        // compute ts
        t0 = 0;
        t1 = pow((Scalar)((p1.x - p0.x) * (p1.x - p0.x)
                          + (p1.y - p0.y) * (p1.y - p0.y)),
                 (Scalar)0.25);
        t2 = t1
             + pow((Scalar)((p2.x - p1.x) * (p2.x - p1.x)
                            + (p2.y - p1.y) * (p2.y - p1.y)),
                   (Scalar)0.25);
        t3 = t2
             + pow((Scalar)((p3.x - p2.x) * (p3.x - p2.x)
                            + (p3.y - p2.y) * (p3.y - p2.y)),
                   (Scalar)0.25);

        // sample between p1 and p2
        for (size_t q = 1; q < n; q++)
        {
            Scalar t = t1 + (t2 - t1) * (Scalar)q / (Scalar)n;

            // compute as
            a1 = p0 * (t1 - t) / (t1 - t0) + p1 * (t - t0) / (t1 - t0);
            a2 = p1 * (t2 - t) / (t2 - t1) + p2 * (t - t1) / (t2 - t1);
            a3 = p2 * (t3 - t) / (t3 - t2) + p3 * (t - t2) / (t3 - t2);

            // compute bs
            b1 = a1 * (t2 - t) / (t2 - t0) + a2 * (t - t0) / (t2 - t0);
            b2 = a2 * (t3 - t) / (t3 - t1) + a3 * (t - t1) / (t3 - t1);

            // compute point
            Point p = b1 * (t2 - t) / (t2 - t1) + b2 * (t - t1) / (t2 - t1);
            newPoints.push_back(p);
        }
    }

    void Path::smooth(float distmult)
    {
        //
        //  Make a new set of points based on the orignals
        //

        PointArray inPoints;
        ScalarArray inWidths;
        ColorArray inColors;
        const bool constWidth = isConstantWidth();
        const bool hasColor = this->hasColor();
        PointArray newPoints;
        ScalarArray newWidths;
        ColorArray newColors;
        const Color black(0, 0, 0, 1);
        const Scalar width = m_width;
        const Scalar width2 = width * width;
        ScalarArray widths = m_widths;

        if (m_points.size() < 2)
            return;

        inPoints.push_back(m_points.front());
        if (!constWidth)
            inWidths.push_back(m_widths.front());
        if (hasColor)
            inColors.push_back(m_colors.front());

        for (size_t i = 1; i < m_points.size(); i++)
        {
            Scalar d = magnitudeSquared(m_points[i] - inPoints.back());

            if (d > Math<Scalar>::epsilon())
            {
                inPoints.push_back(m_points[i]);
                if (!constWidth)
                    inWidths.push_back(m_widths[i]);
                if (hasColor)
                    inColors.push_back(m_colors[i]);
            }
        }

        inPoints.back() = m_points.back();
        if (!constWidth)
            inWidths.back() = m_widths.back();
        if (hasColor)
            inColors.back() = m_colors.back();

        if (inPoints.size() < 3)
            return;

        newPoints.push_back(inPoints.front());
        if (!constWidth)
            newWidths.push_back(inWidths.front());
        if (hasColor)
            newColors.push_back(inColors.front());

        for (size_t i = 1; i < inPoints.size(); i++)
        {
            const Point& p3 = inPoints[i];
            const Point& p0 = inPoints[i - 1];
            const Scalar w3 = constWidth ? width : inWidths[i];
            const Scalar w0 = constWidth ? width : inWidths[i - 1];
            const Color c3 = hasColor ? inColors[i] : black;
            const Color c0 = hasColor ? inColors[i - 1] : black;
            const Vec d = p3 - p0;
            const Scalar dist = magnitude(d);
            const int n = size_t(dist / (min(w3, w0) * distmult));

            const Scalar w = 0.25;

            if (n > 1)
            {
                Vec t0, t1;
                Scalar scale = Scalar(1);

                if (i == 1)
                {
                    sampleCentripetalCatmull(p0 + (p0 - p3), p0, p3,
                                             inPoints[i + 1], n, newPoints);
                }
                else if (i == inPoints.size() - 1)
                {
                    sampleCentripetalCatmull(inPoints[i - 2], p0, p3,
                                             p3 + (p3 - p0), n, newPoints);
                }
                else
                {
                    sampleCentripetalCatmull(inPoints[i - 2], p0, p3,
                                             inPoints[i + 1], n, newPoints);
                }

                for (size_t q = 1; q < n; q++)
                {
                    const Scalar t = Scalar(q) / Scalar(n);

                    if (!constWidth)
                        newWidths.push_back(lerp<Scalar>(w0, w3, t));
                    if (hasColor)
                        newColors.push_back(lerp<Color>(c0, c3, t));
                }

                newPoints.push_back(p3);
                if (!constWidth)
                    newWidths.push_back(w3);
                if (hasColor)
                    newColors.push_back(c3);
            }
            else
            {
                newPoints.push_back(inPoints[i]);
                if (!constWidth)
                    newWidths.push_back(inWidths[i]);
                if (hasColor)
                    newColors.push_back(inColors[i]);
            }
        }

        newPoints.back() = inPoints.back();
        m_points = newPoints;

        if (!constWidth)
        {
            newWidths.back() = inWidths.back();
            m_widths = newWidths;
        }

        if (hasColor)
        {
            newColors.back() = inColors.back();
            m_colors = newColors;
        }
    }

} // namespace TwkPaint
