//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/Exception.h>
#include <IPCore/IPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderFunction.h>
#include <TwkContainer/ImageProperty.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMath/Function.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;
    using namespace TwkContainer;
    using namespace TwkFB;

    IPInstanceNode::IPInstanceNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* graph,
                                   GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_intermediate(false)
    {
        init();
    }

    void IPInstanceNode::init()
    {
        pthread_mutex_init(&m_lock, 0);

        m_activeProperty = declareProperty<IntProperty>("node.active", 1);

        const NodeDefinition* def = definition();

        string p = def->name();
        m_intermediate = def->intValue("render.intermediate");

        if (const Component* c = def->component("parameters"))
        {
            Component* parameters = new Component("parameters");

            //
            //  Add the new component to this Node (PropertyContainer) _before_
            //  we add the parameters (properties) to the component.  Otherwise
            //  the properties do not "inherit" the node as their contiainer, so
            //  they do not have one at all, and things go wrong (in particular
            //  no message is sent by the graph when they are changed).
            //

            add(parameters);
            parameters->copy(c);
        }

        m_numInputs = def->function()->imageParameters().size();
        setMaxInputs(m_numInputs);

        //
        //  Add props for parameters that have none
        //

        const Shader::Function* F = def->function();
        const Shader::SymbolVector& fparams = F->parameters();

        for (size_t i = 0; i < fparams.size(); i++)
        {
            const Shader::Symbol* s = fparams[i];
            string name = "parameters.";
            name += s->name();

            if (s->name() == "frame"
                || (s->name() == "_offset" && s->isSpecial())
                || s->name() == "baseFrame" || s->name() == "stereoEye"
                || F->isInputImageParameter(s->name()))
            {
                //
                //  Skip these. They're "special" parameters -- NOTE: this list
                //  of names is unforunately mirrored below in the bind()
                //  function. It would be nice to have a single location where
                //  these names are determined so that code and this code don't
                //  need to be kept in sync.
                //

                continue;
            }

            if (const Property* p = find(name))
            {
                //
                //  Some of these need to be preprocessed. E.g. data fbs
                //  need to have a hash generated.
                //

                switch (s->type())
                {
                case Shader::Symbol::Sampler1DType:
                case Shader::Symbol::Sampler2DType:
                case Shader::Symbol::Sampler3DType:
                case Shader::Symbol::Sampler2DRectType:
                default:
                    break;
                }
            }
            else
            {
                switch (s->type())
                {
                case Shader::Symbol::FloatType:
                    declareProperty<FloatProperty>(name, 0.0f);
                    break;
                case Shader::Symbol::Vec2fType:
                    declareProperty<Vec2fProperty>(name, Vec2f(0.0f));
                    break;
                case Shader::Symbol::Vec3fType:
                    declareProperty<Vec3fProperty>(name, Vec3f(0.0f));
                    break;
                case Shader::Symbol::Vec4fType:
                    declareProperty<Vec4fProperty>(name, Vec4f(0.0f));
                    break;
                case Shader::Symbol::IntType:
                    declareProperty<IntProperty>(name, 0);
                    break;
                case Shader::Symbol::BoolType:
                    declareProperty<IntProperty>(name, 0);
                    break;
                case Shader::Symbol::Matrix4fType:
                {
                    FloatProperty* p = createProperty<FloatProperty>(name);
                    p->resize(16);
                    (*p)[0] = 1;
                    (*p)[1] = 0;
                    (*p)[2] = 0;
                    (*p)[3] = 0;
                    (*p)[4] = 0;
                    (*p)[5] = 1;
                    (*p)[6] = 0;
                    (*p)[7] = 0;
                    (*p)[8] = 0;
                    (*p)[9] = 0;
                    (*p)[10] = 1;
                    (*p)[11] = 0;
                    (*p)[12] = 0;
                    (*p)[13] = 0;
                    (*p)[14] = 0;
                    (*p)[15] = 1;
                    break;
                }
                case Shader::Symbol::Matrix3fType:
                {
                    FloatProperty* p = createProperty<FloatProperty>(name);
                    p->resize(9);
                    (*p)[0] = 1;
                    (*p)[1] = 0;
                    (*p)[2] = 0;
                    (*p)[3] = 0;
                    (*p)[4] = 1;
                    (*p)[5] = 0;
                    (*p)[6] = 0;
                    (*p)[7] = 0;
                    (*p)[8] = 1;
                    break;
                }
                case Shader::Symbol::Matrix2fType:
                {
                    FloatProperty* p = createProperty<FloatProperty>(name);
                    p->resize(4);
                    (*p)[0] = 1;
                    (*p)[1] = 0;
                    (*p)[2] = 0;
                    (*p)[3] = 1;
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    IPInstanceNode::~IPInstanceNode() { pthread_mutex_destroy(&m_lock); }

    bool IPInstanceNode::isActive() const
    {
        return m_activeProperty->size() == 1 && m_activeProperty->front() == 1;
    }

    bool IPInstanceNode::testInputs(const IPNodes& inputs,
                                    std::ostringstream& msg) const
    {
        if (inputs.size() > m_numInputs)
        {
            string un = uiName();
            string n = name();

            if (un == n)
                msg << n;
            else
                msg << un << " (" << n << ")";
            msg << " requires exactly " << numInputs() << " inputs" << endl;
            return false;
        }
        else
        {
            return IPNode::testInputs(inputs, msg);
        }
    }

    Shader::BoundSymbol* IPInstanceNode::boundSymbolFromSymbol(
        IPImage* image, const Shader::Symbol* s, const Context& context) const
    {
        //
        //  Test for some special cases that override user parameters first
        //

        if ((s->name() == "_offset" || s->name() == "offset")
            && s->type() == Shader::Symbol::Vec2fType && s->isSpecial())
        {
            //
            //  Special parameter (used by renderer) requires initialization to
            //  a constant. This cannot be overriden.
            //

            return new Shader::BoundVec2f(s, Vec2f(0.0f));
        }
        else if (s->type() == Shader::Symbol::OutputImageType)
        {
            return new Shader::BoundSpecial(s);
        }

        string paramName = "parameters.";
        paramName += s->name();

        if (const Property* prop = find(paramName.c_str()))
        {
            if (prop->empty())
                return 0;

            switch (s->type())
            {
            case Shader::Symbol::FloatType:

                if (const FloatProperty* fp =
                        dynamic_cast<const FloatProperty*>(prop))
                {
                    return new Shader::BoundFloat(s, fp->front());
                }

                if (const IntProperty* ip =
                        dynamic_cast<const IntProperty*>(prop))
                {
                    return new Shader::BoundFloat(s, float(ip->front()));
                }
                break;

            case Shader::Symbol::Vec2fType:

                if (const Vec2fProperty* vp =
                        dynamic_cast<const Vec2fProperty*>(prop))
                {
                    return new Shader::BoundVec2f(s, vp->front());
                }
                break;

            case Shader::Symbol::Vec3fType:

                if (const Vec3fProperty* vp =
                        dynamic_cast<const Vec3fProperty*>(prop))
                {
                    return new Shader::BoundVec3f(s, vp->front());
                }
                break;

            case Shader::Symbol::Vec4fType:

                if (const Vec4fProperty* vp =
                        dynamic_cast<const Vec4fProperty*>(prop))
                {
                    return new Shader::BoundVec4f(s, vp->front());
                }
                break;

            case Shader::Symbol::IntType:

                if (const IntProperty* ip =
                        dynamic_cast<const IntProperty*>(prop))
                {
                    return new Shader::BoundInt(s, ip->front());
                }

                if (const FloatProperty* fp =
                        dynamic_cast<const FloatProperty*>(prop))
                {
                    return new Shader::BoundInt(s, int(fp->front()));
                }
                break;

            case Shader::Symbol::BoolType:

                if (const IntProperty* ip =
                        dynamic_cast<const IntProperty*>(prop))
                {
                    return new Shader::BoundBool(s, ip->front());
                }
                break;

            case Shader::Symbol::Matrix4fType:

                if (const Mat44fProperty* m44p =
                        dynamic_cast<const Mat44fProperty*>(prop))
                {
                    return new Shader::BoundMat44f(s, *(Mat44f*)m44p->data());
                }
                else if (const FloatProperty* fp =
                             dynamic_cast<const FloatProperty*>(prop))
                {
                    if (fp->size() == 16)
                    {
                        return new Shader::BoundMat44f(s, *(Mat44f*)fp->data());
                    }
                }
                break;

            case Shader::Symbol::Matrix3fType:

                if (const Mat33fProperty* m33p =
                        dynamic_cast<const Mat33fProperty*>(prop))
                {
                    return new Shader::BoundMat33f(s, *(Mat33f*)m33p->data());
                }
                else if (const FloatProperty* fp =
                             dynamic_cast<const FloatProperty*>(prop))
                {
                    if (fp->size() == 9)
                    {
                        return new Shader::BoundMat33f(s, *(Mat33f*)fp->data());
                    }
                }
                break;

            case Shader::Symbol::Matrix2fType:

                if (const FloatProperty* fp =
                        dynamic_cast<const FloatProperty*>(prop))
                {
                    if (fp->size() == 4)
                    {
                        return new Shader::BoundMat22f(s, *(Mat22f*)fp->data());
                    }
                }
                break;

            case Shader::Symbol::Sampler1DType:

                if (const FloatProperty* fp =
                        dynamic_cast<const FloatProperty*>(prop))
                {
                    // return an FB
                }

                break;

            case Shader::Symbol::Sampler2DType:
                break;

            case Shader::Symbol::Sampler2DRectType:
            {
                const size_t xs = prop->xsizeTrait();
                const size_t ys = prop->ysizeTrait();
                const size_t zs = prop->zsizeTrait();
                const size_t ws = prop->wsizeTrait();

                if (const ImageProperty* ip =
                        dynamic_cast<const ImageProperty*>(prop))
                {
                    if (zs != 0 && ws == 0)
                    {
                        //
                        //    Its a two dimensional image. xs is the
                        //    number of channels (ys, zs) is the width
                        //    and hieght. So the dimensions are:
                        //
                        //        4, 512, 512, 0
                        //
                        //    For a 512x512 4 channel image
                        //
                        //    Make a "disposable" frame buffer that only
                        //    references the data.
                        //

                        FrameBuffer::DataType dtype;

                        switch (ip->layoutTrait())
                        {
                        case Property::FloatLayout:
                            dtype = FrameBuffer::FLOAT;
                            break;
                        case Property::IntLayout:
                            dtype = FrameBuffer::UINT;
                            break;
                        case Property::ShortLayout:
                            dtype = FrameBuffer::USHORT;
                            break;
                        case Property::HalfLayout:
                            dtype = FrameBuffer::HALF;
                            break;
                        case Property::DoubleLayout:
                            dtype = FrameBuffer::DOUBLE;
                            break;
                        default:
                        case Property::ByteLayout:
                            dtype = FrameBuffer::UCHAR;
                            break;
                        }

                        FrameBuffer* fb = new FrameBuffer(
                            FrameBuffer::PixelCoordinates, ys, zs, 0, xs, dtype,
                            const_cast<unsigned char*>(
                                ip->data<unsigned char>()),
                            NULL, FrameBuffer::BOTTOMLEFT, false);

                        fb->idstream() << name() << "+"
                                       << size_t(ip->data<unsigned char>());

                        return new Shader::BoundSampler(
                            s, Shader::ImageOrFB(fb, 0));
                    }
                }
            }

            break;

            case Shader::Symbol::Sampler3DType:
                break;

            default:
                break;
            }
        }

        if (s->name() == "frame")
        {
            //
            //  Current frame. NOTE: because this is *after* the switch
            //  statement above, the user can force the use of a parameter for
            //  frame instead of using the actual frame number. Not sure if
            //  this is a good idea or not.
            //

            if (s->type() == Shader::Symbol::FloatType)
            {
                return new Shader::BoundFloat(s, context.frame);
            }
            else if (s->type() == Shader::Symbol::IntType)
            {
                return new Shader::BoundInt(s, context.frame);
            }
        }
        else if (s->name() == "baseFrame")
        {
            //
            //  context.baseFrame -- similar to above
            //

            if (s->type() == Shader::Symbol::FloatType)
            {
                return new Shader::BoundFloat(s, context.baseFrame);
            }
            else if (s->type() == Shader::Symbol::IntType)
            {
                return new Shader::BoundInt(s, context.baseFrame);
            }
        }
        else if (s->name() == "stereoEye")
        {
            //
            //  context.stereoEye -- similar to above
            //

            if (s->type() == Shader::Symbol::FloatType)
            {
                return new Shader::BoundFloat(s, context.eye);
            }
            else if (s->type() == Shader::Symbol::IntType)
            {
                return new Shader::BoundInt(s, context.eye);
            }
        }
        else if (s->name() == "fps")
        {
            //
            //  context.fps -- similar to above
            //

            if (s->type() == Shader::Symbol::FloatType)
            {
                return new Shader::BoundFloat(s, context.fps);
            }
        }

        return 0;
    }

    Shader::Expression* IPInstanceNode::bind(IPImage* image,
                                             Shader::Expression* expr,
                                             const Context& context) const
    {
        ExprVector exprs(1);
        exprs[0] = expr;
        return bind(image, exprs, context);
    }

    Shader::Expression* IPInstanceNode::bind(IPImage* image,
                                             const ExprVector& exprs,
                                             const Context& context) const
    {
        const Shader::Function* F = definition()->function();
        size_t n = F->parameters().size();
        Shader::ArgumentVector args;
        size_t proxyCount = 0;

        for (size_t i = 0; i < n; i++)
        {
            const Shader::Symbol* p = F->parameters()[i];

            if (p->type() == Shader::Symbol::InputImageType
                || F->isInputImageParameter(p->name()))
            {
                args.push_back(
                    new Shader::BoundExpression(p, exprs[proxyCount]));
                proxyCount++;
            }
            else if (Shader::BoundSymbol* bs =
                         boundSymbolFromSymbol(image, p, context))
            {
                args.push_back(bs);
            }
            else
            {
                // do nothing because we will send values of these parameters
                // from the renderer at runtime
            }
        }

        return new Shader::Expression(F, args, image);
    }

    void IPInstanceNode::addFillerInputs(IPImageVector& images)
    {
        const Shader::Function* F = definition()->function();

        //
        //  Add blank inputs if we don't have the expected amount
        //

        while (images.size() != F->imageParameters().size())
        {
            if (!images.empty())
            {
                images.push_back(IPImage::newBlankImage(this, images[0]->width,
                                                        images[1]->height));
            }
            else
            {
                images.push_back(IPImage::newBlankImage(this, 1280, 720));
            }
        }
    }

} // namespace IPCore
