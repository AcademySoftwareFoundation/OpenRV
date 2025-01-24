//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/qtUtils.h>
#include <MuQt5/Bridge.h>
#include <MuQt5/QTreeWidgetItemType.h>
#include <MuQt5/QListWidgetItemType.h>
#include <MuQt5/QTableWidgetItemType.h>
#include <MuQt5/QStandardItemType.h>
#include <MuQt5/QItemSelectionType.h>
#include <MuQt5/QTreeWidgetType.h>
#include <MuQt5/QListWidgetType.h>
#include <MuQt5/QRectType.h>
#include <MuQt5/QTableWidgetType.h>
#include <MuQt5/QKeySequenceType.h>
#include <MuQt5/QModelIndexType.h>
#include <MuQt5/QUrlType.h>
#include <MuQt5/QKeySequenceType.h>
#include <MuQt5/QIconType.h>
#include <MuQt5/QPointType.h>
#include <MuQt5/QColorType.h>
#include <MuQt5/QPixmapType.h>
#include <MuQt5/QSizeType.h>
#include <MuQt5/QFontType.h>
#include <MuQt5/QVariantType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/Alias.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/Exception.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <Mu/Thread.h>
#include <sstream>

namespace Mu
{
    using namespace std;

    void dumpMetaInfo(const QMetaObject& m)
    {
        const QMetaObject* parent = m.superClass();

        for (int i = 0; i < m.methodCount(); i++)
        {
            QMetaMethod mm = m.method(i);
            if ((parent
                 && parent->indexOfMethod(
                        parent->normalizedSignature(mm.methodSignature()))
                        != -1)
                || mm.methodSignature()[0] == '_')
            {
                continue;
            }

            cout << "method " << i << ": " << mm.methodSignature().constData()
                 << ", " << mm.methodType() << endl;

            QList<QByteArray> pnames = mm.parameterNames();
            QList<QByteArray> ptypes = mm.parameterTypes();

            for (int q = 0; q < pnames.size(); q++)
            {
                cout << "  " << pnames[q].constData() << " : "
                     << ptypes[q].constData() << endl;
            }
        }

        for (int i = 0; i < m.propertyCount(); i++)
        {
            QMetaProperty mp = m.property(i);
            if (parent && parent->indexOfProperty(mp.name()) != -1)
                continue;

            cout << "property " << i << ": " << mp.name() << ", "
                 << mp.typeName() << endl;
        }

        for (int i = 0; i < m.enumeratorCount(); i++)
        {
            QMetaEnum e = m.enumerator(i);
            if (parent && parent->indexOfEnumerator(e.name()) != -1)
                continue;

            cout << "enum " << i << ": " << e.scope() << "." << e.name()
                 << endl;

            for (int q = 0; q < e.keyCount(); q++)
            {
                cout << "  " << e.key(q) << " = " << e.keyToValue(e.key(q))
                     << endl;
            }
        }
    }

    const char* qtType(const Mu::Type* t)
    {
        Name tname = t->fullyQualifiedName();

        if (tname == "string")
            return "const QString&";
        if (tname == "int")
            return "int";
        if (tname == "bool")
            return "bool";
        if (tname == "double")
            return "qreal";
        if (tname == "qt.QIcon")
            return "QIcon";
        if (tname == "qt.QSize")
            return "QSize";
        if (tname == "qt.QRect")
            return "QRect";
        if (tname == "qt.QPoint")
            return "QPoint";
        if (tname == "qt.QUrl")
            return "QUrl";
        if (tname == "qt.QColor")
            return "QColor";
        if (tname == "qt.QFont")
            return "QFont";
        if (tname == "qt.QAction")
            return "QAction*";
        if (tname == "qt.QTreeWidgetItem")
            return "QTreeWidgetItem*";
        if (tname == "qt.QListWidgetItem")
            return "QListWidgetItem*";
        if (tname == "qt.QTableWidgetItem")
            return "QTableWidgetItem*";
        if (tname == "qt.QStandardItem")
            return "QStandardItem*";
        if (tname == "qt.QModelIndex")
            return "QModelIndex";
        if (tname == "qt.QKeySequence")
            return "QKeySequence";
        if (tname == "qt.QPixmap")
            return "QPixmap";
        if (tname == "qt.QItemSelection")
            return "QItemSelection";
        return 0;
    }

    static QMetaProperty getprop(const Node& NODE_THIS, Thread& NODE_THREAD,
                                 QObject* o)
    {
        const Symbol* s = NODE_THIS.symbol();
        string n = s->name();
        const char* np = n.c_str();

        if (n.find("set") == 0 && n.size() > 3 && n[3] >= 'A' && n[3] <= 'Z')
        {
            n[3] = n[3] - 'A' + 'a';
            np += 3;
        }

        const QMetaObject* metaObject = o->metaObject();
        int propIndex = metaObject->indexOfProperty(np);
        return metaObject->property(propIndex);
    }

    static QMetaMethod getmethod(const Node& NODE_THIS, Thread& NODE_THREAD,
                                 QObject* o)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        vector<const char*> args;

        ostringstream str;
        str << F->name();
        str << "(";
        bool hasInt = false;

        for (size_t i = 1; i < F->numArgs(); i++)
        {
            string tname = F->argType(i)->fullyQualifiedName().c_str();
            const char* aname = 0;
            if (i > 1)
                str << ",";

            if (tname == "int")
            {
                aname = "int";
                hasInt = true;
            }
            else if (tname == "bool")
            {
                aname = "bool";
            }
            else if (tname == "string")
            {
                aname = "QString";
            }
            else if (tname == "qt.QIcon")
            {
                aname = "QIcon";
            }
            else if (tname == "qt.QSize")
            {
                aname = "QSize";
            }
            else if (tname == "qt.QRect")
            {
                aname = "QRect";
            }
            else if (tname == "qt.QPoint")
            {
                aname = "QPoint";
            }
            else if (tname == "qt.QFont")
            {
                aname = "QFont";
            }
            else if (tname == "qt.QUrl")
            {
                aname = "QUrl";
            }
            else if (tname == "qt.QColor")
            {
                aname = "QColor";
            }
            else if (tname == "qt.QAction")
            {
                aname = "QAction*";
            }
            else if (tname == "qt.QTreeWidgetItem")
            {
                aname = "QTreeWidgetItem*";
            }
            else if (tname == "qt.QListWidgetItem")
            {
                aname = "QListWidgetItem*";
            }
            else if (tname == "qt.QTableWidgetItem")
            {
                aname = "QTableWidgetItem*";
            }
            else if (tname == "qt.QStandardItem")
            {
                aname = "QStandardItem*";
            }
            else if (tname == "qt.QModelIndex")
            {
                aname = "QModelIndex";
            }
            else if (tname == "qt.QKeySequence")
            {
                aname = "QKeySequence";
            }
            else if (tname == "qt.QPixmap")
            {
                aname = "QPixmap";
            }
            else if (tname == "qt.QItemSelection")
            {
                aname = "QItemSelection";
            }
            else if (tname == "double")
            {
                aname = "qreal";
            }

            if (aname)
            {
                args.push_back(aname);
                str << aname;
            }
            else
            {
                cout << "ERROR: MuQt Bridge unsupported method arg type "
                     << aname << endl;
            }
        }

        str << ")";

        const QMetaObject* metaObject = o->metaObject();
        int methodIndex = metaObject->indexOfMethod(str.str().c_str());

        if (methodIndex == -1 && hasInt)
        {
            //
            //  For flag args, MuQt uses a brain dead "int" for the
            //  type. So if we called to find the method and there's an
            //  int in the args, then see if we can match against a *Flags
            //  arg and use that instead.
            //

            ostringstream rstr;
            rstr << F->name() << "\\(";
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i)
                    rstr << ",";

                if (!strcmp(args[i], "int"))
                    rstr << "[a-zA-Z0-9_:]+Flags?";
                else
                    rstr << args[i];
            }

            rstr << "\\)";

            QRegExp re(rstr.str().c_str());

            for (size_t i = 0; i < metaObject->methodCount(); i++)
            {
                if (re.exactMatch(metaObject->method(i).methodSignature()))
                {
                    methodIndex = i;
                    // cout << "MATCHED: " << metaObject->method(i).signature()
                    // << endl;
                    break;
                }
            }
        }

        if (methodIndex == -1)
        {
            cout << "ERROR: MuQt Bridge getmethod failed for " << str.str()
                 << endl;
            cout << "       methods are:" << endl;

            for (size_t i = 0; i < metaObject->methodCount(); i++)
            {
                cout << "       "
                     << metaObject->method(i).methodSignature().constData()
                     << endl;
            }
        }

        return metaObject->method(methodIndex);
    }

    //----------------------------------------------------------------------
    //  PROPERTY GET/SET

    static NODE_IMPLEMENTATION(getpropInt, int)
    {
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        NODE_RETURN(v.toInt());
    }

    static NODE_IMPLEMENTATION(getpropBool, bool)
    {
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        NODE_RETURN(v.toBool());
    }

    static NODE_IMPLEMENTATION(getpropDouble, double)
    {
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        NODE_RETURN(v.toDouble());
    }

    static NODE_IMPLEMENTATION(getpropString, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        StringType::String* str =
            c->stringType()->allocate(v.toString().toUtf8().constData());
        NODE_RETURN(str);
    }

    static NODE_IMPLEMENTATION(getpropIcon, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QIconType* iconType =
            c->findSymbolOfTypeByQualifiedName<QIconType>(
                c->internName("qt.QIcon"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        QIconType::Instance* i = new QIconType::Instance(iconType);

        if (v.canConvert(QVariant::Icon))
        {
            i->value = v.value<QIcon>();
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropSize, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QSizeType* sizeType =
            c->findSymbolOfTypeByQualifiedName<QSizeType>(
                c->internName("qt.QSize"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Size))
        {
            i = makeqtype<QSizeType>(sizeType, v.value<QSize>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropRect, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QRectType* rectType =
            c->findSymbolOfTypeByQualifiedName<QRectType>(
                c->internName("qt.QRect"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Rect))
        {
            i = makeqtype<QRectType>(rectType, v.value<QRect>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropPoint, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPointType* pointType =
            c->findSymbolOfTypeByQualifiedName<QPointType>(
                c->internName("qt.QPoint"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Point))
        {
            i = makeqtype<QPointType>(pointType, v.value<QPoint>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropFont, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QFontType* type = c->findSymbolOfTypeByQualifiedName<QFontType>(
            c->internName("qt.QFont"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Font))
        {
            i = makeqtype<QFontType>(type, v.value<QFont>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropUrl, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QUrlType* type = c->findSymbolOfTypeByQualifiedName<QUrlType>(
            c->internName("qt.QUrl"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Url))
        {
            i = makeqtype<QUrlType>(type, v.value<QUrl>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropKeySeq, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QKeySequenceType* type =
            c->findSymbolOfTypeByQualifiedName<QKeySequenceType>(
                c->internName("qt.QKeySequence"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::KeySequence))
        {
            i = makeqtype<QKeySequenceType>(type, v.value<QKeySequence>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(getpropColor, Pointer)
    {
        const MuLangContext* c =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const QColorType* type = c->findSymbolOfTypeByQualifiedName<QColorType>(
            c->internName("qt.QColor"), false);
        QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
        QVariant v = prop.read(o);
        ClassInstance* i = 0;

        if (v.canConvert(QVariant::Color))
        {
            i = makeqtype<QColorType>(type, v.value<QColor>());
        }

        NODE_RETURN(i);
    }

    static NODE_IMPLEMENTATION(putpropInt, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            prop.write(o, QVariant(NODE_ARG(1, int)));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropBool, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            prop.write(o, QVariant(NODE_ARG(1, bool)));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropDouble, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            prop.write(o, QVariant(NODE_ARG(1, double)));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropString, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            if (StringType::String* s = NODE_ARG_OBJECT(1, StringType::String))
            {
                prop.write(o, QVariant(s->c_str()));
            }
            else
            {
                throw NilArgumentException(NODE_THREAD);
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropIcon, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QIcon value = getqtype<QIconType>(i);
            prop.write(o, QVariant(value));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropSize, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QSize s = getqtype<QSizeType>(i);
            prop.write(o, QVariant(s));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropPoint, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QPoint point = getqtype<QPointType>(i);
            prop.write(o, QVariant(point));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropRect, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QRect rect = getqtype<QRectType>(i);
            prop.write(o, QVariant(rect));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropFont, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QFont value = getqtype<QFontType>(i);
            prop.write(o, QVariant(value));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropUrl, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QUrl value = getqtype<QUrlType>(i);
            prop.write(o, QVariant(value));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropKeySeq, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QKeySequence value = getqtype<QKeySequenceType>(i);
            QVariant v;
            v.setValue(value);
            prop.write(o, v);
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(putpropColor, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaProperty prop = getprop(NODE_THIS, NODE_THREAD, o);
            ClassInstance* i = NODE_ARG_OBJECT(1, ClassInstance);
            QColor value = getqtype<QColorType>(i);
            prop.write(o, QVariant(value));
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    //----------------------------------------------------------------------
    //  METHOD

    struct QActionArg
    {
        QAction* a;
    };

    struct QObjectArg
    {
        QObject* a;
    };

    QGenericArgument argument(STLVector<Pointer>::Type& gcCache, const Type* T,
                              Value& v, QString& s, QVariant& qv)
    {
        //
        //  NOTE: this function is tricky. It has to return a ValueType**
        //  as a QGenericArgument using the Q_ARG macro. The problem is
        //  that you have to be pointing at memory that survives this
        //  function invocation. In the case of QString and primitive
        //  types, an object on the callers stack is passed in to make the
        //  happen, otherwise the heap location must be referenced
        //  directly (not through a local).
        //

        if (T->name() == "int")
        {
            return Q_ARG(int, v._int);
        }
        else if (T->name() == "bool")
        {
            return Q_ARG(bool, v._bool);
        }
        else if (T->name() == "double")
        {
            return Q_ARG(qreal, v._double);
        }
        else if (T->name() == "string")
        {
            StringType::String* str =
                reinterpret_cast<StringType::String*>(v._Pointer);
            s = str->c_str();
            return Q_ARG(QString, s);
        }

        else if (T->name() == "QVariant")
        {
            QVariantType::Instance* tvar =
                reinterpret_cast<QVariantType::Instance*>(v._Pointer);
            qv = tvar->value;
            return Q_ARG(QVariant, qv);
        }

        else if (T->name() == "QAction")
        {
            if (ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer))
            {
                return Q_ARG(QAction*, i->data<QActionArg>()->a);
            }
        }
        else if (T->name() == "QObject")
        {
            if (ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer))
            {
                return Q_ARG(QObject*, i->data<QObjectArg>()->a);
            }
        }

        else if (T->name() == "QColor")
        {
            return Q_ARG(
                QColor,
                reinterpret_cast<QColorType::Instance*>(v._Pointer)->value);
        }
        else if (T->name() == "QPoint")
        {
            return Q_ARG(
                QPoint,
                reinterpret_cast<QPointType::Instance*>(v._Pointer)->value);
        }
        else if (T->name() == "QUrl")
        {
            return Q_ARG(
                QUrl, reinterpret_cast<QUrlType::Instance*>(v._Pointer)->value);
        }
        else if (T->name() == "QModelIndex")
        {
            return Q_ARG(
                QModelIndex,
                reinterpret_cast<QModelIndexType::Instance*>(v._Pointer)
                    ->value);
        }
        else if (T->name() == "QKeySequence")
        {
            return Q_ARG(
                QKeySequence,
                reinterpret_cast<QKeySequenceType::Instance*>(v._Pointer)
                    ->value);
        }
        else if (T->name() == "QPixmap")
        {
            return Q_ARG(
                QPixmap,
                reinterpret_cast<QPixmapType::Instance*>(v._Pointer)->value);
        }
        else if (T->name() == "QItemSelection")
        {
            return Q_ARG(
                QItemSelection,
                reinterpret_cast<QItemSelectionType::Instance*>(v._Pointer)
                    ->value);
        }
        else if (T->name() == "QTreeWidgetItem")
        {
            ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer);
            return Q_ARG(QTreeWidgetItem*,
                         i->data<QTreeWidgetItemType::Struct>()->object);
        }
        else if (T->name() == "QListWidgetItem")
        {
            ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer);
            return Q_ARG(QListWidgetItem*,
                         i->data<QListWidgetItemType::Struct>()->object);
        }
        else if (T->name() == "QTableWidgetItem")
        {
            ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer);
            return Q_ARG(QTableWidgetItem*,
                         i->data<QTableWidgetItemType::Struct>()->object);
        }
        else if (T->name() == "QStandardItem")
        {
            ClassInstance* i = reinterpret_cast<ClassInstance*>(v._Pointer);
            return Q_ARG(QStandardItem*,
                         i->data<QStandardItemType::Struct>()->object);
        }
        else
        {
            cout << "argument: T->name() = " << T->name() << endl;
        }

        return Q_ARG(int, 0);
    }

    static NODE_IMPLEMENTATION(invokeMethod0, void)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            bool result = method.invoke(o, Qt::DirectConnection);

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod0 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod0_bool, bool)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            bool rval;
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            bool result = method.invoke(o, Qt::DirectConnection,
                                        Q_RETURN_ARG(bool, rval));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod0 failed for "
                     << method.methodSignature().constData() << endl;
            }

            NODE_RETURN(rval);
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod0_int, int)
    {
        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            int rval;
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            bool result =
                method.invoke(o, Qt::DirectConnection, Q_RETURN_ARG(int, rval));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod0 failed for "
                     << method.methodSignature().constData() << endl;
            }

            NODE_RETURN(rval);
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod1, void)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);

            Value p0 = NODE_ANY_TYPE_ARG(1);
            STLVector<Pointer>::Type gcCache;
            QString s;
            QVariant v;

            bool result =
                method.invoke(o, Qt::DirectConnection,
                              argument(gcCache, F->argType(1), p0, s, v));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod1 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod2, void)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            Value p0 = NODE_ANY_TYPE_ARG(1);
            Value p1 = NODE_ANY_TYPE_ARG(2);
            STLVector<Pointer>::Type gcCache;
            QString s1, s2;
            QVariant v1, v2;

            bool result =
                method.invoke(o, Qt::DirectConnection,
                              argument(gcCache, F->argType(1), p0, s1, v1),
                              argument(gcCache, F->argType(2), p1, s2, v2));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod2 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod3, void)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            Value p0 = NODE_ANY_TYPE_ARG(1);
            Value p1 = NODE_ANY_TYPE_ARG(2);
            Value p2 = NODE_ANY_TYPE_ARG(3);
            STLVector<Pointer>::Type gcCache;
            QString s1, s2, s3;
            QVariant v1, v2, v3;

            bool result =
                method.invoke(o, Qt::DirectConnection,
                              argument(gcCache, F->argType(1), p0, s1, v1),
                              argument(gcCache, F->argType(2), p1, s2, v2),
                              argument(gcCache, F->argType(3), p2, s3, v3));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod3 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod4, void)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            Value p0 = NODE_ANY_TYPE_ARG(1);
            Value p1 = NODE_ANY_TYPE_ARG(2);
            Value p2 = NODE_ANY_TYPE_ARG(3);
            Value p3 = NODE_ANY_TYPE_ARG(4);
            STLVector<Pointer>::Type gcCache;
            QString s1, s2, s3, s4;
            QVariant v1, v2, v3, v4;

            bool result =
                method.invoke(o, Qt::DirectConnection,
                              argument(gcCache, F->argType(1), p0, s1, v1),
                              argument(gcCache, F->argType(2), p1, s2, v2),
                              argument(gcCache, F->argType(3), p2, s3, v3),
                              argument(gcCache, F->argType(4), p3, s4, v4));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod4 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    static NODE_IMPLEMENTATION(invokeMethod5, void)
    {
        const Function* F = static_cast<const Function*>(NODE_THIS.symbol());

        if (QObject* o = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance)))
        {
            QMetaMethod method = getmethod(NODE_THIS, NODE_THREAD, o);
            Value p0 = NODE_ANY_TYPE_ARG(1);
            Value p1 = NODE_ANY_TYPE_ARG(2);
            Value p2 = NODE_ANY_TYPE_ARG(3);
            Value p3 = NODE_ANY_TYPE_ARG(4);
            Value p4 = NODE_ANY_TYPE_ARG(5);
            STLVector<Pointer>::Type gcCache;
            QString s1, s2, s3, s4, s5;
            QVariant v1, v2, v3, v4, v5;

            bool result =
                method.invoke(o, Qt::DirectConnection,
                              argument(gcCache, F->argType(1), p0, s1, v1),
                              argument(gcCache, F->argType(2), p1, s2, v2),
                              argument(gcCache, F->argType(3), p2, s3, v3),
                              argument(gcCache, F->argType(4), p3, s4, v4),
                              argument(gcCache, F->argType(5), p4, s5, v5));

            if (!result)
            {
                cout << "ERROR: MuQt Bridge invokeMethod5 failed for "
                     << method.methodSignature().constData() << endl;
            }
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    //----------------------------------------------------------------------

    void populate(Class* c, const QMetaObject& m, const char** propExclusions)
    {
        bool verbose = false;

        // if (!strcmp(m.className(), "QDockWidget")) verbose = true;

        if (verbose)
        {
            cout << "POPULATE: " << m.className() << endl;
        }

        MuLangContext* context = static_cast<MuLangContext*>(c->context());

        const QMetaObject* parent = m.superClass();
        QIconType* iconType =
            context->findSymbolOfTypeByQualifiedName<QIconType>(
                context->internName("qt.QIcon"), false);
        QSizeType* sizeType =
            context->findSymbolOfTypeByQualifiedName<QSizeType>(
                context->internName("qt.QSize"), false);

        QPointType* pointType =
            context->findSymbolOfTypeByQualifiedName<QPointType>(
                context->internName("qt.QPoint"), false);

        QRectType* rectType =
            context->findSymbolOfTypeByQualifiedName<QRectType>(
                context->internName("qt.QRect"), false);

        QFontType* fontType =
            context->findSymbolOfTypeByQualifiedName<QFontType>(
                context->internName("qt.QFont"), false);

        QUrlType* urlType = context->findSymbolOfTypeByQualifiedName<QUrlType>(
            context->internName("qt.QUrl"), false);

        QColorType* colorType =
            context->findSymbolOfTypeByQualifiedName<QColorType>(
                context->internName("qt.QColor"), false);

        QKeySequenceType* keysequenceType =
            context->findSymbolOfTypeByQualifiedName<QKeySequenceType>(
                context->internName("qt.QKeySequence"), false);

        // Class* actionType =
        //     context->findSymbolOfTypeByQualifiedName<Class>(context->internName("qt.QAction"),
        //                                                     false);

        // Class* treeWidgetItemType =
        //     context->findSymbolOfTypeByQualifiedName<Class>(context->internName("qt.QTreeWidgetItem"),
        //                                                     false);

        // Class* listWidgetItemType =
        //     context->findSymbolOfTypeByQualifiedName<Class>(context->internName("qt.QListWidgetItem"),
        //                                                     false);

        // Class* tableWidgetItemType =
        //     context->findSymbolOfTypeByQualifiedName<Class>(context->internName("qt.QTableWidgetItem"),
        //                                                     false);

        assert(colorType);

        for (int i = 0; i < m.enumeratorCount(); i++)
        {
            QMetaEnum e = m.enumerator(i);
            if (parent && parent->indexOfEnumerator(e.name()) != -1)
                continue;

            c->addSymbol(new Alias(context, e.name(), "int"));

            for (int q = 0; q < e.keyCount(); q++)
            {
                SymbolicConstant* sc = new SymbolicConstant(
                    context, e.key(q), "int", Value(e.keyToValue(e.key(q))));

                c->addSymbol(sc);

                if (verbose)
                {
                    cout << "ENUM: ";
                    sc->output(cout);
                    cout << endl;
                }
            }
        }

        for (int i = 0; i < m.propertyCount(); i++)
        {
            QMetaProperty mp = m.property(i);
            if (parent && parent->indexOfProperty(mp.name()) != -1)
                continue;

            string getname = mp.name();
            string setname = "set";
            setname += mp.name();
            setname[3] = setname[3] - 'a' + 'A';

            bool getexclude = false;
            bool setexclude = false;

            if (propExclusions)
            {
                for (const char** e = propExclusions; *e; e++)
                {
                    if (getname == *e)
                        getexclude = true;
                    if (setname == *e)
                        setexclude = true;
                }
            }

            const char* t = 0;
            const Type* rtype = 0;
            NodeFunc getfunc = NodeFunc();
            NodeFunc putfunc = NodeFunc();
            STLVector<ParameterVariable*>::Type params(2);
            params[0] = new ParameterVariable(context, "this", c);
            params[1] = 0;

            switch (mp.type())
            {
            case QVariant::Int:
                t = "int";
                rtype = context->intType();
                getfunc = getpropInt;
                putfunc = putpropInt;
                break;
            case QVariant::String:
                t = "string";
                rtype = context->stringType();
                getfunc = getpropString;
                putfunc = putpropString;
                break;
            case QVariant::Bool:
                t = "bool";
                rtype = context->boolType();
                getfunc = getpropBool;
                putfunc = putpropBool;
                break;
            case QVariant::Double:
                t = "double";
                rtype = context->doubleType();
                getfunc = getpropDouble;
                putfunc = putpropDouble;
                break;
            case QVariant::Icon:
                t = "qt.QIcon";
                rtype = iconType;
                getfunc = getpropIcon;
                putfunc = putpropIcon;
                break;
            case QVariant::Size:
                t = "qt.QSize";
                rtype = sizeType;
                getfunc = getpropSize;
                putfunc = putpropSize;
                break;
            case QVariant::Point:
                t = "qt.QPoint";
                rtype = pointType;
                getfunc = getpropPoint;
                putfunc = putpropPoint;
                break;
            case QVariant::Rect:
                t = "qt.QRect";
                rtype = rectType;
                getfunc = getpropRect;
                putfunc = putpropRect;
                break;
            case QVariant::Font:
                t = "qt.QFont";
                rtype = fontType;
                getfunc = getpropFont;
                putfunc = putpropFont;
                break;
            case QVariant::Url:
                t = "qt.QUrl";
                rtype = urlType;
                getfunc = getpropUrl;
                putfunc = putpropUrl;
                break;
            case QVariant::KeySequence:
                t = "qt.QKeySequence";
                rtype = keysequenceType;
                getfunc = getpropKeySeq;
                putfunc = putpropKeySeq;
                break;
            case QVariant::Color:
                t = "qt.QColor";
                rtype = colorType;
                getfunc = getpropColor;
                putfunc = putpropColor;
                break;
            case QVariant::UserType:
                if (mp.isEnumType())
                {
                    t = "int";
                    rtype = context->intType();
                    getfunc = getpropInt;
                    putfunc = putpropInt;
                }
                break;
            default:
                break;
            }

            if (t)
            {
                if (mp.isReadable() && !getexclude)
                {
                    Function* F =
                        new Function(context, getname.c_str(), rtype, 1,
                                     &params.front(), getfunc, Function::None);
                    c->addSymbol(F);

                    if (verbose)
                    {
                        cout << "PROP GET: ";
                        F->output(cout);
                        cout << endl;
                    }
                }

                if (mp.isWritable() && !setexclude)
                {

                    params[0] = new ParameterVariable(context, "this", c);
                    params[1] = new ParameterVariable(context, "value", rtype);

                    Function* F = new Function(
                        context, setname.c_str(), context->voidType(), 2,
                        &params.front(), putfunc, Function::None);
                    c->addSymbol(F);

                    if (verbose)
                    {
                        cout << "PROP PUT: ";
                        F->output(cout);
                        cout << endl;
                    }
                }
            }
            else
            {
                if (verbose)
                {
                    cout << "SKIPPING: property " << getname << " (+ "
                         << setname << ") : " << mp.typeName() << " in class "
                         << m.className() << endl;
                }
            }
        }

        for (int i = 0; i < m.methodCount(); i++)
        {
            QMetaMethod mm = m.method(i);
            bool isBoolType = !strcmp(mm.typeName(), "bool");
            bool isIntType = !strcmp(mm.typeName(), "int");

            if ((parent
                 && parent->indexOfMethod(
                        parent->normalizedSignature(mm.methodSignature()))
                        != -1)
                || mm.methodSignature()[0] == '_')
            {
                continue;
            }

            STLVector<ParameterVariable*>::Type params;
            params.push_back(new ParameterVariable(context, "this", c));

            string fname = mm.methodSignature().constData();
            string::size_type ip = fname.find('(');
            fname.erase(ip, fname.size() - ip);
            QList<QByteArray> pnames = mm.parameterNames();
            QList<QByteArray> ptypes = mm.parameterTypes();

            bool failed = false;

            for (size_t i = 0; i < pnames.size(); i++)
            {
                string n = pnames[i].constData();
                string t = ptypes[i].constData();

                if (t == "int" || t.find("::") != string::npos)
                {
                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           context->intType()));
                }
                else if (t == "QString")
                {
                    params.push_back(new ParameterVariable(
                        context, n.c_str(), context->stringType()));
                }
                else if (t == "QIcon")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), iconType));
                }
                else if (t == "QSize")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), sizeType));
                }
                else if (t == "QRect")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), rectType));
                }
                else if (t == "QPoint")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), pointType));
                }
                else if (t == "QFont")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), fontType));
                }
                else if (t == "QUrl")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), urlType));
                }
                else if (t == "QColor")
                {
                    params.push_back(
                        new ParameterVariable(context, n.c_str(), colorType));
                }
                else if (t == "qreal")
                {
                    params.push_back(new ParameterVariable(
                        context, n.c_str(), context->doubleType()));
                }
                else if (t == "bool")
                {
                    params.push_back(new ParameterVariable(
                        context, n.c_str(), context->boolType()));
                }
                else if (t.find("QAction*") != string::npos)
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           "qt.QAction"));
                }
                else if (t.find("QTreeWidgetItem*") != string::npos)
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(
                        context, n.c_str(), "qt.QTreeWidgetItem"));
                }
                else if (t.find("QListWidgetItem*") != string::npos)
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(
                        context, n.c_str(), "qt.QListWidgetItem"));
                }
                else if (t.find("QTableWidgetItem*") != string::npos)
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(
                        context, n.c_str(), "qt.QTableWidgetItem"));
                }
                else if (t.find("QStandardItem*") != string::npos)
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           "qt.QStandardItem"));
                }
                else if (t == "QModelIndex")
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           "qt.QModelIndex"));
                }
                else if (t == "QKeySequence")
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           "qt.QKeySequence"));
                }
                else if (t == "QPixmap")
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(context, n.c_str(),
                                                           "qt.QPixmap"));
                }
                else if (t == "QItemSelection")
                {
                    if (n == "")
                    {
                        ostringstream str;
                        str << "param" << i;
                        n = str.str();
                    }

                    params.push_back(new ParameterVariable(
                        context, n.c_str(), "qt.QItemSelection"));
                }
                else
                {
                    failed = true;
                    // cout << "failed arg: " << t << endl;
                    break;
                }
            }

            NodeFunc func = NodeFunc();

            switch (params.size())
            {
            case 1:
                // only methods with return types are bool, int, and QVariant
                if (isBoolType)
                    func = invokeMethod0_bool;
                else if (isIntType)
                    func = invokeMethod0_int;
                else
                    func = invokeMethod0;
                break;
            case 2:
                func = invokeMethod1;
                break;
            case 3:
                func = invokeMethod2;
                break;
            case 4:
                func = invokeMethod3;
                break;
            case 5:
                func = invokeMethod4;
                break;
            case 6:
                func = invokeMethod5;
                break;
            default:
                failed = true;
                break;
            }

            if (!failed)
            {
                const Type* rtype = context->voidType();
                if (isBoolType)
                    rtype = context->boolType();
                else if (isIntType)
                    rtype = context->intType();

                Function* F =
                    new Function(context, fname.c_str(), rtype, params.size(),
                                 &params.front(), func, Function::None);

                c->addSymbol(F);

                if (verbose)
                {
                    cout << "METHOD: ";
                    F->output(cout);
                    cout << endl;
                }
            }
            else
            {
                if (verbose)
                {
                    cout << "SKIPPING METHOD: " << fname << "(";

                    for (size_t i = 0; i < pnames.size(); i++)
                    {
                        string n = pnames[i].constData();
                        string t = ptypes[i].constData();
                        if (i)
                            cout << ", ";
                        cout << t << " " << n;
                    }

                    cout << ")" << endl;
                }
            }
        }
    }

} // namespace Mu
