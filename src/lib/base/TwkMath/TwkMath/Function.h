// Copyright (c) 2001 Tweak Inc. All rights reserved.

#ifndef _TwkMathFunction_h_
#define _TwkMathFunction_h_

#include <TwkMath/Math.h>
#include <TwkMath/PlaneAlgo.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/dll_defs.h>
#include <algorithm>

namespace TwkMath
{

    template <typename T> inline T sqr(const T& a) { return a * a; }

    template <typename T> inline T cube(const T& a) { return a * a * a; }

    template <typename T> inline T wrap(const T& a, const T& by)
    {
        T rem = a % by;
        return rem < ((T)0) ? rem + by : rem;
    }

    template <typename T> inline T betterMod(const T& a, const T& by)
    {
        return wrap(a, by);
    }

    template <typename T> inline T degToRad(const T& deg)
    {
        return Math<T>::degToRad(deg);
    }

    template <typename T> inline T radToDeg(const T& rad)
    {
        return Math<T>::radToDeg(rad);
    }

    template <typename T, typename INTERP>
    inline T lerp(const T& lo, const T& hi, const INTERP& interp)
    {
        // CJH:
        // After having lunch with Larry Gritz, he explained
        // to me that this:
        // lo + interp * ( hi - lo );
        // is a really bad bad way of implementing this function.
        // That's because the precision loss of the (hi - lo)
        // statement is potentially massive if they're far apart
        // from each other.
        // By doing:
        // lo * ( 1 - interp ) + hi * interp
        // instead, you limit the precision loss.
        // BAD: return lo + interp * ( hi - lo );
        // GOOD:
        return T((lo * (((INTERP)1) - interp)) + (hi * interp));
    }

    // Smoothstep function
    // Goes from 0 to 1.
    template <typename T> inline T smoothstep(const T& t)
    {
        if (t <= T(0))
        {
            return T(0);
        }
        else if (t >= T(1))
        {
            return T(1);
        }
        else
        {
            return t * t * (T(3) - t * T(2));
        }
    }

    template <typename T> inline T gradSmoothstep(const T& t)
    {
        if (t <= T(0) || t >= T(1))
        {
            return T(0);
        }
        else
        {
            return T(6) * t * (T(1) - t);
        }
    }

    template <typename T>
    inline T smoothstep(const T& edge0, const T& edge1, const T& t)
    {
        if (edge0 == edge1)
            return t < edge0 ? T(0) : T(1); // step function
        return smoothstep((t - edge0) / (edge1 - edge0));
    }

    template <typename T>
    inline T gradSmoothstep(const T& edge0, const T& edge1, const T& t)
    {
        if (edge0 == edge1)
            return T(0);
        return gradSmoothstep((t - edge0) / (edge1 - edge0));
    }

    template <typename T> inline T gauss(const T& x)
    {
        return Math<T>::expf(-(x * x));
    }

    // RGB to unit hsv
    template <typename T>
    void rgbToHsv(const T& r, const T& g, const T& b, T& h, T& s, T& v)
    {
        // Find the minimum and maximum values.
        T minVal = Math<T>::max();
        T maxVal = Math<T>::min();

        // Check red.
        minVal = std::min(minVal, r);
        maxVal = std::max(maxVal, r);

        // Check green.
        minVal = std::min(minVal, g);
        maxVal = std::max(maxVal, g);

        // Check blue.
        minVal = std::min(minVal, b);
        maxVal = std::max(maxVal, b);

        const T delta = maxVal - minVal;

        // Set value, initialize others.
        h = (T)0;
        s = (T)0;
        v = maxVal;

        // Set saturation.
        if (maxVal == (T)0)
        {
            s = (T)0;
        }
        else
        {
            s = delta / maxVal;
        }

        // Set hue.
        // Only bother if saturation is non-zero,
        // for which hue would be undefined, and can be left
        // at zero
        if (s != (T)0)
        {
            // If max is red...
            if (r == maxVal)
            {
                // Resulting color is between yellow and magenta.
                h = (g - b) / delta;
            }
            else if (g == maxVal) // If max is green
            {
                // Resulting color is between cyan and yellow
                h = ((T)2) + (b - r) / delta;
            }
            else
            {
                // Resulting color is between magenta and cyan.
                h = ((T)4) + (r - g) / delta;
            }

            // Convert to unit scale.
            h /= (T)6;

            // Prevent negative hues.
            if (h < (T)0)
            {
                h += (T)1;
            }
        }
    }

    // unit HSV to RGB
    template <typename T>
    void hsvToRgb(const T& h, const T& s, const T& v, T& r, T& g, T& b)
    {
        if (s <= (T)0)
        {
            r = v;
            g = v;
            b = v;
        }
        else
        {
            const T h6 = h * (T)6;
            int i = (int)(Math<T>::floor(h6));
            if (i > 5)
            {
                i = 0;
            }

            const T f = h6 - (T)i;

            const T p = v * (((T)1) - s);
            const T q = v * (((T)1) - (s * f));
            const T t = v * (((T)1) - (s * (((T)1) - f)));

            switch (i)
            {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            case 5:
            default:
                r = v;
                g = p;
                b = q;
                break;
            }
        }
    }

    template <typename T> inline bool zeroTol(const T& v, const T& tol)
    {
        return (Math<T>::abs(v) < tol);
    }

    template <typename T>
    inline bool equalTol(const T& a, const T& b, const T& tol)
    {
        return (Math<T>::abs(a - b) < tol);
    }

    template <typename T>
    inline T areaOfTriangle(const Vec3<T>& va, const Vec3<T>& vb,
                            const Vec3<T>& vc)
    {
        Vec3<T> v0 = vb - va;
        Vec3<T> v1 = vc - va;
        Vec3<T> c = cross(v0, v1);
        return c.magnitude() / T(2);
    }

    template <typename T>
    inline T min4(const T& a1, const T& a2, const T& a3, const T& a4)
    {
        return std::min(std::min(std::min(a1, a2), a3), a4);
    }

    template <typename T>
    inline T max4(const T& a1, const T& a2, const T& a3, const T& a4)
    {
        return std::max(std::max(std::max(a1, a2), a3), a4);
    }

    template <typename T> inline bool isPowerOfTwo(T val)
    {
        T count = ((T)0);
        while (val != ((T)0))
        {
            if (val & 1)
                ++count;
            val = val >> 1;
        }
        return count == 1;
    }

    template <typename T> inline T taperedClamp(T val, T lo, T hi)
    {
        if (val < T(0))
            val = lo;
        const T u = (val - lo) / (hi - lo);
        return lerp(lo, hi,
                    (Math<T>::exp(T(0)) - Math<T>::exp(-u))
                        / (Math<T>::exp(T(0)) - Math<T>::exp(T(-1))));
    }

    template <typename T> static const T trilinear(T v[2][2][2], float t[3])
    {
        T top, bottom, front, back;

        //
        // Interpolate first in the "front" plane
        //

        bottom = lerp(v[0][0][0], v[0][0][1], t[0]); // x interpolation
        top = lerp(v[0][1][0], v[0][1][1], t[0]);
        front = lerp(bottom, top, t[1]); // y interpolation

        //
        // now interpolate in the "back" plane
        //

        bottom = lerp(v[1][0][0], v[1][0][1], t[0]); // x interpolation
        top = lerp(v[1][1][0], v[1][1][1], t[0]);
        back = lerp(bottom, top, t[1]); // y interpolation

        //
        // Finally, interpolate between "front" and "back"
        //

        return lerp(front, back, t[2]);
    }

    TWKMATH_EXPORT size_t gcd(size_t u, size_t v);

    //
    //  Intersection of line that contains a and b with line that contains
    //  c and d
    //

    template <typename T>
    Vec2<T> intersectionOfLines(const Vec2<T>& a, const Vec2<T>& b,
                                const Vec2<T>& c, const Vec2<T>& d)
    {
        const T x1 = a.x;
        const T x2 = b.x;
        const T x3 = c.x;
        const T x4 = d.x;

        const T y1 = a.y;
        const T y2 = b.y;
        const T y3 = c.y;
        const T y4 = d.y;

        const T mx0 = x1 - x2;
        const T mx1 = x3 - x4;
        const T my0 = y1 - y2;
        const T my1 = y3 - y4;

        const T d0 = x1 * y2 - y1 * x2;
        const T d1 = x3 * y4 - y3 * x4;
        const T d2 = mx0 * my1 - my0 * mx1;

        return Vec2<T>((d0 * mx1 - mx0 * d1) / d2, (d0 * my1 - my0 * d1) / d2);
    }

} // End namespace TwkMath

#endif
