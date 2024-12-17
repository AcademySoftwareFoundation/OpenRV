//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMatExtract_h_
#define _TwkMathMatExtract_h_

#include <TwkMath/Vec3.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Exc.h>
#include <TwkMath/Math.h>

namespace TwkMath
{

    //******************************************************************************
    template <typename T> inline Mat44<T> xRotationMatrix(const T& xRot)
    {
        T cosr = Math<T>::cos(xRot);
        T sinr = Math<T>::sin(xRot);

        return Mat44<T>(T(1), T(0), T(0), T(0), T(0), cosr, -sinr, T(0), T(0),
                        sinr, cosr, T(0), T(0), T(0), T(0), T(1));
    }

    //******************************************************************************
    template <typename T>
    void extractEulerXYZ(const Mat44<T>& mat, Vec3<T>& rot)
    {
        // Normalize the local x, y and z axes to remove scaling.
        Vec3<T> i(mat[0][0], mat[1][0], mat[2][0]);
        Vec3<T> j(mat[0][1], mat[1][1], mat[2][1]);
        Vec3<T> k(mat[0][2], mat[1][2], mat[2][2]);

        i.normalize();
        j.normalize();
        k.normalize();

        Mat44<T> M(i[0], j[0], k[0], T(0), i[1], j[1], k[1], T(0), i[2], j[2],
                   k[2], T(0), T(0), T(0), T(0), T(1));

        // Extract the first angle, rot.x.
        rot.x = Math<T>::atan2(M[2][1], M[2][2]);

        // Remove the rot.x rotation from M, so that the remaining
        // rotation, N, is only around two axes, and gimbal lock
        // cannot occur.
        Mat44<T> N = xRotationMatrix(-rot.x);
        N = M * N;

        // Extract the other two angles, rot.y and rot.z, from N.
        T cy = Math<T>::sqrt(N[0][0] * N[0][0] + N[1][0] * N[1][0]);

        rot.y = Math<T>::atan2(-N[2][0], cy);
        rot.z = Math<T>::atan2(-N[0][1], N[1][1]);
    }

    //******************************************************************************
    template <typename T>
    bool checkForZeroScaleInCol(const T& scl, const Vec3<T>& col,
                                bool throwOnError)
    {
        for (int i = 0; i < 3; ++i)
        {
            if ((Math<T>::abs(scl) < T(1)
                 && Math<T>::abs(col[i]) >= Math<T>::max() * scl))
            {
                if (throwOnError)
                {
                    TWK_EXC_THROW_WHAT(
                        ZeroScaleExc,
                        "Cannot remove zero scaling from matrix.");
                }
                else
                {
                    return false;
                }
            }
        }

        return true;
    }

    //******************************************************************************
    template <typename T>
    bool extractAndRemoveScalingAndShear(Mat44<T>& mat, Vec3<T>& scl,
                                         Vec3<T>& shr, bool throwOnError)
    {
        Vec3<T> col[3];

        col[0] = Vec3<T>(mat[0][0], mat[1][0], mat[2][0]);
        col[1] = Vec3<T>(mat[0][1], mat[1][1], mat[2][1]);
        col[2] = Vec3<T>(mat[0][2], mat[1][2], mat[2][2]);

        T maxVal = 0;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (Math<T>::abs(col[i][j]) > maxVal)
                {
                    maxVal = Math<T>::abs(col[i][j]);
                }
            }
        }

        if (maxVal != T(0))
        {
            for (int i = 0; i < 3; ++i)
            {
                if (!checkForZeroScaleInCol(maxVal, col[i], throwOnError))
                {
                    return false;
                }
                else
                {
                    col[i] /= maxVal;
                }
            }
        }

        // Compute X scale factor.
        scl.x = col[0].magnitude();
        if (!checkForZeroScaleInCol(scl.x, col[0], throwOnError))
        {
            return false;
        }

        // Normalize first col.
        col[0] /= scl.x;

        // An XY shear factor will shear the X coord. as the Y coord. changes.
        // There are 6 combinations (XY, XZ, YZ, YX, ZX, ZY), although we only
        // extract the first 3 because we can effect the last 3 by shearing in
        // XY, XZ, YZ combined rotations and scales.
        //
        // shear matrix <   1,  YX,  ZX,  0,
        //                 XY,   1,  ZY,  0,
        //                 XZ,  YZ,   1,  0,
        //                  0,   0,   0,  1 >

        // Compute XY shear factor and make 2nd col orthogonal to 1st.
        shr[0] = dot(col[0], col[1]);
        col[1] -= shr[0] * col[0];

        // Now, compute Y scale.
        scl.y = col[1].magnitude();
        if (!checkForZeroScaleInCol(scl.y, col[1], throwOnError))
        {
            return false;
        }

        // Normalize 2nd col and correct the XY shear factor for Y scaling.
        col[1] /= scl.y;
        shr[0] /= scl.y;

        // Compute XZ and YZ shears, orthogonalize 3rd col.
        shr[1] = dot(col[0], col[2]);
        col[2] -= shr[1] * col[0];
        shr[2] = dot(col[1], col[2]);
        col[2] -= shr[2] * col[1];

        // Next, get Z scale.
        scl.z = col[2].magnitude();
        if (!checkForZeroScaleInCol(scl.z, col[2], throwOnError))
        {
            return false;
        }

        // Normalize 3rd col and correct the XZ and YZ shear factors for Z
        // scaling.
        col[2] /= scl.z;
        shr[1] /= scl.z;
        shr[2] /= scl.z;

        // At this point, the upper 3x3 matrix in mat is orthonormal.
        // Check for a coordinate system flip. If the determinant
        // is less than zero, then negate the matrix and the scaling factors.
        if (dot(col[0], cross(col[1], col[2])) < 0)
        {
            for (int i = 0; i < 3; ++i)
            {
                scl[i] *= -T(1);
                col[i] *= -T(1);
            }
        }

        // Copy over the orthonormal cols into the returned matrix.
        // The upper 3x3 matrix in mat is now a rotation matrix.
        for (int i = 0; i < 3; ++i)
        {
            mat[0][i] = col[i][0];
            mat[1][i] = col[i][1];
            mat[2][i] = col[i][2];
        }

        scl *= maxVal;

        return true;
    }

    //******************************************************************************
    template <typename T>
    bool extractSHRT(const Mat44<T>& mat, Vec3<T>& Scale, Vec3<T>& sHear,
                     Vec3<T>& Rotation, Vec3<T>& Translation,
                     bool THROW_ON_ERROR)
    {
        Mat44<T> rot = mat;
        if (!extractAndRemoveScalingAndShear(rot, Scale, sHear, THROW_ON_ERROR))
        {
            return false;
        }

        extractEulerXYZ(rot, Rotation);

        Translation.x = mat[0][3];
        Translation.y = mat[1][3];
        Translation.z = mat[2][3];

        return true;
    }

    //******************************************************************************
    template <typename T>
    Mat44<T> alignZAxisWithTargetDir(Vec3<T> targetDir, Vec3<T> upDir)
    {
        // Ensure that the target direction is non-zero.
        if (targetDir.magnitude() == T(0))
        {
            targetDir = Vec3<T>(T(0), T(0), T(1));
        }

        // Ensure that the up direction is non-zero.
        if (upDir.magnitude() == T(0))
        {
            upDir = Vec3<T>(T(0), T(1), T(0));
        }

        // Check for degeneracies.  If the upDir and targetDir are parallel
        // or opposite, then compute a new, arbitrary up direction that is
        // not parallel or opposite to the targetDir.
        if (cross(upDir, targetDir).magnitude() == T(0))
        {
            upDir = cross(targetDir, Vec3<T>(T(1), T(0), T(0)));
            if (upDir.magnitude() == T(0))
            {
                upDir = cross(targetDir, Vec3<T>(T(0), T(0), T(1)));
            }
        }

        // Compute the x-, y-, and z-axis vectors of the new coordinate system.
        Vec3<T> targetPerpDir = cross(upDir, targetDir);
        Vec3<T> targetUpDir = cross(targetDir, targetPerpDir);

        // Rotate the x-axis into targetPerpDir (col 0),
        // rotate the y-axis into targetUpDir   (col 1),
        // rotate the z-axis into targetDir     (col 2).
        Vec3<T> col[3];
        col[0] = targetPerpDir.normalized();
        col[1] = targetUpDir.normalized();
        col[2] = targetDir.normalized();

        return Mat44<T>(col[0][0], col[1][0], col[2][0], T(0), col[0][1],
                        col[1][1], col[2][1], T(0), col[0][2], col[1][2],
                        col[2][2], T(0), T(0), T(0), T(0), T(1));
    }

    //******************************************************************************
    template <typename T>
    Mat44<T> rotationMatrixWithUpDir(const Vec3<T>& fromDir,
                                     const Vec3<T>& toDir, const Vec3<T>& upDir)
    {
        // The goal is to obtain a rotation matrix that takes
        // "fromDir" to "toDir".  We do this in two steps and
        // compose the resulting rotation matrices;
        //    (a) rotate "fromDir" into the z-axis
        //    (b) rotate the z-axis into "toDir"

        // The from direction must be non-zero; but we allow zero to and up
        // dirs.
        if (fromDir.magnitude() == T(0))
        {
            Mat44<T> ret;
            ret.makeIdentity();
            return ret;
        }
        else
        {
            Mat44<T> zAxis2FromDir =
                alignZAxisWithTargetDir(fromDir, Vec3<T>(T(0), T(1), T(0)));

            Mat44<T> fromDir2zAxis = zAxis2FromDir.transposed();

            Mat44<T> zAxis2ToDir = alignZAxisWithTargetDir(toDir, upDir);

            return zAxis2ToDir * fromDir2zAxis;
        }
    }

} // End namespace TwkMath

#endif
