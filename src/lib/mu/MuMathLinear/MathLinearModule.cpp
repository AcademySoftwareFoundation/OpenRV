//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuMathLinear/MathLinearModule.h>
#include <Mu/Function.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/Exception.h>
#include <Mu/ParameterVariable.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/FixedArray.h>
#include <ImathMatrix.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace Mu
{
    using namespace std;

    MathLinearModule::MathLinearModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    MathLinearModule::~MathLinearModule() {}

    namespace
    {

        //
        //  Mu fixed arrays and Imath matrices both store their elements
        //  row-major, so the values can be copied verbatim.
        //

        template <typename M> M loadMatrix(const float* data)
        {
            M m;
            memcpy(&m.x[0][0], data, sizeof(m.x));
            return m;
        }

        template <typename M> void storeMatrix(const M& m, float* data) { memcpy(data, &m.x[0][0], sizeof(m.x)); }

        void makeIdentity(float* m, size_t n)
        {
            fill(m, m + n * n, 0.0f);

            for (size_t i = 0; i < n; i++)
                m[i * n + i] = 1.0f;
        }

        //
        //  Gauss-Jordan elimination with partial pivoting for arbitrarily sized
        //  matrices. A singular matrix produces the identity, which is what
        //  Imath's gjInverse() does for the fixed size cases below.
        //

        void invertSquareMatrix(const float* in, float* out, size_t n)
        {
            vector<float> t(in, in + n * n);
            vector<float> s(n * n);
            makeIdentity(s.data(), n);

            for (size_t i = 0; i < n; i++)
            {
                size_t pivot = i;
                float pivotSize = fabsf(t[i * n + i]);

                for (size_t j = i + 1; j < n; j++)
                {
                    const float mag = fabsf(t[j * n + i]);

                    if (mag > pivotSize)
                    {
                        pivot = j;
                        pivotSize = mag;
                    }
                }

                if (pivotSize == 0.0f)
                {
                    makeIdentity(out, n);
                    return;
                }

                if (pivot != i)
                {
                    for (size_t j = 0; j < n; j++)
                    {
                        swap(t[i * n + j], t[pivot * n + j]);
                        swap(s[i * n + j], s[pivot * n + j]);
                    }
                }

                const float d = t[i * n + i];

                for (size_t j = 0; j < n; j++)
                {
                    t[i * n + j] /= d;
                    s[i * n + j] /= d;
                }

                for (size_t j = 0; j < n; j++)
                {
                    if (j == i)
                        continue;

                    const float f = t[j * n + i];

                    for (size_t k = 0; k < n; k++)
                    {
                        t[j * n + k] -= f * t[i * n + k];
                        s[j * n + k] -= f * s[i * n + k];
                    }
                }
            }

            copy(s.begin(), s.end(), out);
        }

        //
        //  The generic inverse/transpose functions return the same type as their
        //  argument, so they can only operate on square two dimensional arrays.
        //  Returns 0 for anything else.
        //

        size_t squareMatrixSize(const FixedArray* array)
        {
            const FixedArrayType::SizeVector& dims = array->arrayType()->dimensions();

            if (dims.size() != 2 || dims[0] != dims[1])
                return 0;

            return dims[0];
        }

    } // namespace

    NODE_IMPLEMENTATION(mult_m44_m44, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        FixedArray* Barray = NODE_ARG_OBJECT(1, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const Imath::M44f A = loadMatrix<Imath::M44f>(Aarray->data<float>());
        const Imath::M44f B = loadMatrix<Imath::M44f>(Barray->data<float>());

        storeMatrix(A * B, Carray->data<float>());

        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(mult_m44_v4, Vector4f)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        Mu::Vector4f v = NODE_ARG(1, Mu::Vector4f);
        const Class* mtype = static_cast<const Class*>(Aarray->type());

        const float* m = Aarray->data<float>();

        Vector4f r =
            newVector(m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3], m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3],
                      m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3], m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3]);

        NODE_RETURN(r);
    }

    NODE_IMPLEMENTATION(mult_m44_v3, Vector3f)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        Mu::Vector3f v = NODE_ARG(1, Mu::Vector3f);
        const Class* mtype = static_cast<const Class*>(Aarray->type());

        const float* m = Aarray->data<float>();

        float x = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
        float y = m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7];
        float z = m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11];
        float w = m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15];

        NODE_RETURN(newVector(x / w, y / w, z / w));
    }

    NODE_IMPLEMENTATION(mult_m33_m33, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        FixedArray* Barray = NODE_ARG_OBJECT(1, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const Imath::M33f A = loadMatrix<Imath::M33f>(Aarray->data<float>());
        const Imath::M33f B = loadMatrix<Imath::M33f>(Barray->data<float>());

        storeMatrix(A * B, Carray->data<float>());
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_m33, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const Imath::M33f A = loadMatrix<Imath::M33f>(Aarray->data<float>());

        storeMatrix(A.gjInverse(), Carray->data<float>());
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_m44, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const Imath::M44f A = loadMatrix<Imath::M44f>(Aarray->data<float>());

        storeMatrix(A.gjInverse(), Carray->data<float>());
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_mXX, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const size_t n = squareMatrixSize(Aarray);
        float* C = Carray->data<float>();

        if (n == 0)
        {
            fill(C, C + Carray->size(), 0.0f);
            NODE_RETURN(Carray);
        }

        invertSquareMatrix(Aarray->data<float>(), C, n);
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(transpose_mXX, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray = static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        const size_t n = squareMatrixSize(Aarray);
        const float* A = Aarray->data<float>();
        float* C = Carray->data<float>();

        if (n == 0)
        {
            fill(C, C + Carray->size(), 0.0f);
            NODE_RETURN(Carray);
        }

        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n; j++)
                C[j * n + i] = A[i * n + j];
        }

        NODE_RETURN(Carray);
    }

    //----------------------------------------------------------------------

    class FixedMatrixFunction : public Function
    {
    public:
        FixedMatrixFunction(Context* context, const char* name, NodeFunc, Attributes attributes, ...);

        virtual const Type* nodeReturnType(const Node*) const;
    };

    FixedMatrixFunction::FixedMatrixFunction(Context* context, const char* name, NodeFunc func, Attributes attrs, ...)
        : Function(context, name)
    {
        va_list ap;
        va_start(ap, attrs);
        init(func, attrs, ap);
        va_end(ap);
    }

    const Type* FixedMatrixFunction::nodeReturnType(const Node* node) const { return node->argNode(0)->type(); }

    //----------------------------------------------------------------------

    void MathLinearModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        MuLangContext* c = (MuLangContext*)globalModule()->context();

        c->arrayType(c->floatType(), 2, 4, 4);
        c->arrayType(c->floatType(), 2, 3, 3);

        //
        //  Specialized functions
        //

        globalScope()->addSymbols(
            new Function(c, "*", mult_m44_m44, Op, Return, "float[4,4]", Args, "float[4,4]", "float[4,4]", End),

            new Function(c, "*", mult_m33_m33, Op, Return, "float[3,3]", Args, "float[3,3]", "float[3,3]", End),

            new Function(c, "*", mult_m44_v4, Op, Return, "vector float[4]", Args, "float[4,4]", "vector float[4]", End),

            new Function(c, "*", mult_m44_v3, Op, Return, "vector float[3]", Args, "float[4,4]", "vector float[3]", End),

            new Function(c, "inverse", inverse_m44, Mapped, Return, "float[4,4]", Args, "float[4,4]", End),

            new Function(c, "inverse", inverse_m33, Mapped, Return, "float[3,3]", Args, "float[3,3]", End),

            new FixedMatrixFunction(c, "inverse", inverse_mXX, Mapped, Return, "?fixed_array", Args, "?fixed_array", End),

            new FixedMatrixFunction(c, "transpose", transpose_mXX, Mapped, Return, "?fixed_array", Args, "?fixed_array", End),

            EndArguments);
    }

} // namespace Mu
