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
#include <algorithm>
#include <Eigen/Eigen>

namespace Mu
{
    using namespace std;

    // USING_PART_OF_NAMESPACE_EIGEN

    MathLinearModule::MathLinearModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    MathLinearModule::~MathLinearModule() {}

    typedef Eigen::Map<Eigen::Matrix<float, 4, 4, Eigen::RowMajor>> EigenMat44f;
    typedef Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::RowMajor>> EigenMat33f;
    typedef Eigen::Map<
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>
        EigenMatXf;

    NODE_IMPLEMENTATION(mult_m44_m44, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        FixedArray* Barray = NODE_ARG_OBJECT(1, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMat44f A(Aarray->data<float>());
        EigenMat44f B(Barray->data<float>());
        EigenMat44f C(Carray->data<float>());

        C = A * B;

        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(mult_m44_v4, Vector4f)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        Mu::Vector4f v = NODE_ARG(1, Mu::Vector4f);
        const Class* mtype = static_cast<const Class*>(Aarray->type());

        const float* m = Aarray->data<float>();

        Vector4f r = newVector(
            m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3],
            m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3],
            m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3],
            m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3]);

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
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMat33f A(Aarray->data<float>());
        EigenMat33f B(Barray->data<float>());
        EigenMat33f C(Carray->data<float>());

        C = A * B;
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_m33, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMat33f A(Aarray->data<float>());
        EigenMat33f C(Carray->data<float>());

        C = A.inverse();
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_m44, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMat44f A(Aarray->data<float>());
        EigenMat44f C(Carray->data<float>());

        C = A.inverse();
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(inverse_mXX, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMatXf A(Aarray->data<float>(), Aarray->size(0), Aarray->size(1));
        EigenMatXf C(Carray->data<float>(), Aarray->size(0), Aarray->size(1));

        C = A.inverse();
        NODE_RETURN(Carray);
    }

    NODE_IMPLEMENTATION(transpose_mXX, Pointer)
    {
        FixedArray* Aarray = NODE_ARG_OBJECT(0, FixedArray);
        const Class* mtype = static_cast<const Class*>(Aarray->type());
        FixedArray* Carray =
            static_cast<FixedArray*>(ClassInstance::allocate(mtype));

        EigenMatXf A(Aarray->data<float>(), Aarray->size(0), Aarray->size(1));
        EigenMatXf C(Carray->data<float>(), Aarray->size(0), Aarray->size(1));

        C = A.transpose();
        NODE_RETURN(Carray);
    }

    //----------------------------------------------------------------------

    class FixedMatrixFunction : public Function
    {
    public:
        FixedMatrixFunction(Context* context, const char* name, NodeFunc,
                            Attributes attributes, ...);

        virtual const Type* nodeReturnType(const Node*) const;
    };

    FixedMatrixFunction::FixedMatrixFunction(Context* context, const char* name,
                                             NodeFunc func, Attributes attrs,
                                             ...)
        : Function(context, name)
    {
        va_list ap;
        va_start(ap, attrs);
        init(func, attrs, ap);
        va_end(ap);
    }

    const Type* FixedMatrixFunction::nodeReturnType(const Node* node) const
    {
        return node->argNode(0)->type();
    }

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
            new Function(c, "*", mult_m44_m44, Op, Return, "float[4,4]", Args,
                         "float[4,4]", "float[4,4]", End),

            new Function(c, "*", mult_m33_m33, Op, Return, "float[3,3]", Args,
                         "float[3,3]", "float[3,3]", End),

            new Function(c, "*", mult_m44_v4, Op, Return, "vector float[4]",
                         Args, "float[4,4]", "vector float[4]", End),

            new Function(c, "*", mult_m44_v3, Op, Return, "vector float[3]",
                         Args, "float[4,4]", "vector float[3]", End),

            new Function(c, "inverse", inverse_m44, Mapped, Return,
                         "float[4,4]", Args, "float[4,4]", End),

            new Function(c, "inverse", inverse_m33, Mapped, Return,
                         "float[3,3]", Args, "float[3,3]", End),

            new FixedMatrixFunction(c, "inverse", inverse_mXX, Mapped, Return,
                                    "?fixed_array", Args, "?fixed_array", End),

            new FixedMatrixFunction(c, "transpose", transpose_mXX, Mapped,
                                    Return, "?fixed_array", Args,
                                    "?fixed_array", End),

            EndArguments);
    }

} // namespace Mu
