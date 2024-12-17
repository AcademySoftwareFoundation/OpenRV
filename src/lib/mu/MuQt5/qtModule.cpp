//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include <QtUiTools/QtUiTools>
#include <QtWebEngineWidgets/QWebEngineView>
#include <MuQt5/Bridge.h>
#include <MuQt5/MuQt.h>
#include <MuQt5/SignalSpy.h>
#include <Mu/Alias.h>
#include <Mu/Function.h>
#include <Mu/Thread.h>
#include <Mu/MuProcess.h>
#include <Mu/FunctionObject.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/ParameterVariable.h>
#include <Mu/OpaqueType.h>
#include <MuLang/StringType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/ExceptionType.h>
#include <sstream>

namespace Mu
{
    using namespace std;

    struct SpyData
    {
        SpyData(SignalSpy* s, const Function* f)
            : spy(s)
            , func(f)
        {
        }

        SignalSpy* spy;
        const Function* func;
    };

    typedef STLVector<SpyData>::Type SpyDataVector;

    //
    //  These need to be visible to the GC since they hold
    //
    static SpyDataVector allSpyData;

    qtModule::qtModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    qtModule::~qtModule() {}

    static const char* signalName(const string& method, vector<char>& buffer,
                                  vector<QMetaMethod>& methods)
    {
        //
        //  NOTE: this code is unwrapping the Qt macro SIGNAL. If they
        //  change it this code will also have to change.
        //
        //  If exactly one method was found it must be the one, so use its
        //  signature. This makes it possible to connect to signals with
        //  flag arguments without having to know the name of the enum;
        //  the types are all checked via Mu symbols to make sure they
        //  already match.
        //

        buffer.push_back('2');

        if (methods.size() == 1)
        {
            QByteArray a = methods[0].methodSignature();
            std::copy(a.begin(), a.end(), back_inserter(buffer));
        }
        else
        {
            std::copy(method.begin(), method.end(), back_inserter(buffer));
        }

#ifdef QT_NO_DEBUG
        buffer.push_back(0);
        return &buffer.front();
#else
        buffer.push_back(0);
        buffer.push_back('X');
        buffer.push_back(':');
        buffer.push_back('1');
        return qFlagLocation(&buffer.front());
#endif
    }

    static void findSignal(const QMetaObject* metaObject, const Function* F,
                           vector<QMetaMethod>& methods)
    {
        for (size_t i = 0; i < metaObject->methodCount(); i++)
        {
            QMetaMethod method = metaObject->method(i);

            if (method.methodType() == QMetaMethod::Signal)
            {
                string sig = method.methodSignature().constData();
                string::size_type n = sig.find("(");

                if (n != string::npos)
                {
                    string name = sig.substr(0, n);

                    if (name == F->name().c_str()
                        && method.parameterTypes().size() == (F->numArgs() - 1))
                    {
                        methods.push_back(method);
                    }
                }
            }
        }
    }

    static void findSignalByName(const QMetaObject* metaObject,
                                 const char* name, const Function* F,
                                 vector<QMetaMethod>& methods)
    {
        for (size_t i = 0; i < metaObject->methodCount(); i++)
        {
            QMetaMethod method = metaObject->method(i);

            if (method.methodType() == QMetaMethod::Signal)
            {
                string sig = method.methodSignature().constData();
                string::size_type n = sig.find("(");

                if (n != string::npos)
                {
                    string methodName = sig.substr(0, n);

                    if (name == methodName
                        && F->numArgs() == method.parameterCount())
                    {
                        methods.push_back(method);
                    }
                }
            }
        }
    }

    static void showSignals(const QMetaObject* metaObject, const Function* F,
                            ostream& o)
    {
        for (size_t i = 0; i < metaObject->methodCount(); i++)
        {
            QMetaMethod method = metaObject->method(i);

            if (method.methodType() == QMetaMethod::Signal)
            {
                string sig = method.methodSignature().constData();
                string::size_type n = sig.find("(");

                if (n != string::npos)
                {
                    string name = sig.substr(0, n);
                    o << "   -> " << method.methodSignature().constData()
                      << endl;
                }
            }
        }
    }

    static NODE_IMPLEMENTATION(connect, int)
    {
        Process* process = NODE_THREAD.process();
        QObject* sender = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        FunctionObject* signalObj = NODE_ARG_OBJECT(1, FunctionObject);
        FunctionObject* slotObj = NODE_ARG_OBJECT(2, FunctionObject);
        const Function* Fsig = signalObj->function();
        const Function* Fslot = slotObj->function();

        if (!sender)
            throw NilArgumentException(NODE_THREAD);

        bool ok = Fsig->returnType()->match(Fslot->returnType())
                  && Fsig->numArgs() - 1 == Fslot->numArgs();

        for (size_t i = 0; ok && i < Fslot->numArgs(); i++)
        {
            ok = Fslot->argType(i)->match(Fsig->argType(i + 1));
        }

        int rval = 0;
        vector<QMetaMethod> methods;
        findSignal(sender->metaObject(), Fsig, methods);

        if (ok && !methods.empty())
        {
            ostringstream str;
            str << Fsig->name() << "(";

            for (size_t i = 1; i < Fsig->numArgs(); i++)
            {
                if (i != 1)
                    str << ",";
                if (const char* name = qtType(Fsig->argType(i)))
                    str << name;
            }

            str << ")";

            vector<char> nameBuffer, n2;

#if 0
        cout << "connect signal: " << signalName(str.str(), n2) 
             << " of " << sender->metaObject()->className()
             << endl;
#endif

            const char* sname = signalName(str.str(), nameBuffer, methods);

            SignalSpy* spy = new SignalSpy(sender, sname, Fslot, process);
            allSpyData.push_back(SpyData(spy, Fslot));
            rval = allSpyData.size() - 1;
        }
        else
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* context = (MuLangContext*)p->context();
            ostringstream str;
            str << "qt.connect: function ";
            Fslot->output(str);

            if (methods.empty())
            {
                str << " cannot be connected because ";
                Fsig->output(str);
                str << " is not a signal";

                str << ". Possible signals for "
                    << sender->metaObject()->className() << " are:" << endl;
                showSignals(sender->metaObject(), Fsig, str);
            }
            else
            {
                str << " is incompatible with signal ";
                Fsig->output(str);
            }

            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() += str.str().c_str();
            NODE_THREAD.setException(e);
            ProgramException exc(NODE_THREAD);
            exc.message() = str.str().c_str();
            throw exc;
        }

        NODE_RETURN(rval);
    }

    static NODE_IMPLEMENTATION(connect2, int)
    {
        Process* process = NODE_THREAD.process();
        QObject* sender = object<QObject>(NODE_ARG_OBJECT(0, ClassInstance));
        StringType::String* sigName = NODE_ARG_OBJECT(1, StringType::String);
        FunctionObject* slotObj = NODE_ARG_OBJECT(2, FunctionObject);
        const Function* Fslot = slotObj->function();
        int rval = 0;

        vector<char> nameBuffer;
        vector<QMetaMethod> methods;

        string inName = sigName->c_str();

        if (inName.find('(') == string::npos)
        {
            findSignalByName(sender->metaObject(), inName.c_str(),
                             slotObj->function(), methods);
            if (methods.size() > 0)
                inName = methods[0].methodSignature().constData();
        }

        SignalSpy* spy = new SignalSpy(
            sender, signalName(inName.c_str(), nameBuffer, methods), Fslot,
            process);

        allSpyData.push_back(SpyData(spy, Fslot));
        rval = allSpyData.size() - 1;

        NODE_RETURN(rval);
    }

    static NODE_IMPLEMENTATION(loadUIFile, Pointer)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        StringType::String* filename = NODE_ARG_OBJECT(0, StringType::String);
        QWidget* parent = object<QWidget>(NODE_ARG_OBJECT(1, ClassInstance));

        QUiLoader loader;
        QFile file(filename->c_str());
        file.open(QFile::ReadOnly);
        QWidget* w = loader.load(&file, parent);
        file.close();

        // Note that we cannot instantiate a QWebEngineView widget via a .ui
        // file to conform with LGPL licensing. Because the Qt QWebEngineView
        // plugin is GPL due to its QtDesigner dependency. So to work around
        // this inconvenience we will manually replace 'webView' QWidget with
        // QWebEngineView if any.
        if (w)
        {
            QWidget* from = w->findChild<QWidget*>("webView");
            if (from && !dynamic_cast<QWebEngineView*>(from))
            {
                QWebEngineView* webView =
                    new QWebEngineView(from->parentWidget());
                webView->setObjectName("webView");
                from->parentWidget()->layout()->replaceWidget(from, webView);
                delete from;
                from = nullptr;
            }
        }

        QWidgetType* wtype = c->findSymbolOfTypeByQualifiedName<QWidgetType>(
            c->internName("qt.QWidget"), false);

        ClassInstance* obj = ClassInstance::allocate(wtype);
        setobject(obj, w);
        NODE_RETURN(obj);
    }

    static NODE_IMPLEMENTATION(qobjectEquality, bool)
    {
        QObject* arg0 = object<QObject>(NODE_ARG(0, Pointer));
        QObject* arg1 = object<QObject>(NODE_ARG(1, Pointer));
        NODE_RETURN(arg0 == arg1);
    }

    static NODE_IMPLEMENTATION(qobjectInequality, bool)
    {
        QObject* arg0 = object<QObject>(NODE_ARG(0, Pointer));
        QObject* arg1 = object<QObject>(NODE_ARG(1, Pointer));
        NODE_RETURN(arg0 != arg1);
    }

    static NODE_IMPLEMENTATION(qobjectExists, bool)
    {
        QObject* arg0 = object<QObject>(NODE_ARG(0, Pointer));
        NODE_RETURN(arg0 != 0);
    }

    static NODE_IMPLEMENTATION(qinvoke, bool)
    {
        STLVector<Pointer>::Type gcCache;
        QObject* arg0 = object<QObject>(NODE_ARG(0, Pointer));
        const int ctype = NODE_ARG(1, int);
        const StringType::String* name = NODE_ARG_OBJECT(2, StringType::String);

        if (!arg0 || !name)
            throw NilArgumentException(NODE_THREAD);

        const QMetaObject* metaObject = arg0->metaObject();
        const int index = metaObject->indexOfMethod(name->c_str());

        if (index == -1)
            throw BadArgumentException(NODE_THREAD);

        QMetaMethod method = metaObject->method(index);
        const size_t numArgs = NODE_NUM_ARGS() - 3;

        QString s;
        bool result;
        Qt::ConnectionType connectionType;

        switch (ctype)
        {
        default:
        case 0:
            connectionType = Qt::AutoConnection;
            break;
        case 1:
            connectionType = Qt::DirectConnection;
            break;
        case 2:
            connectionType = Qt::QueuedConnection;
            break;
        case 3:
            connectionType = Qt::BlockingQueuedConnection;
            break;
        case 4:
            connectionType = Qt::UniqueConnection;
            break;
        }

        switch (numArgs)
        {
        case 0:
        {
            result = method.invoke(arg0, connectionType);
            break;
        }
        case 1:
        {
            QVariant v;
            const Type* t0 = NODE_THIS.argNode(3)->type();
            Value p0 = NODE_ANY_TYPE_ARG(3);
            result = method.invoke(arg0, connectionType,
                                   argument(gcCache, t0, p0, s, v));
            break;
        }
        case 2:
        {
            QVariant v1;
            QVariant v2;
            const Type* t0 = NODE_THIS.argNode(3)->type();
            const Type* t1 = NODE_THIS.argNode(4)->type();
            Value p0 = NODE_ANY_TYPE_ARG(3);
            Value p1 = NODE_ANY_TYPE_ARG(4);
            result = method.invoke(arg0, connectionType,
                                   argument(gcCache, t0, p0, s, v1),
                                   argument(gcCache, t1, p1, s, v2));
            break;
        }
        case 3:
        {
            QVariant v1;
            QVariant v2;
            QVariant v3;
            const Type* t0 = NODE_THIS.argNode(3)->type();
            const Type* t1 = NODE_THIS.argNode(4)->type();
            const Type* t2 = NODE_THIS.argNode(5)->type();
            Value p0 = NODE_ANY_TYPE_ARG(3);
            Value p1 = NODE_ANY_TYPE_ARG(4);
            Value p2 = NODE_ANY_TYPE_ARG(5);
            result = method.invoke(arg0, connectionType,
                                   argument(gcCache, t0, p0, s, v1),
                                   argument(gcCache, t1, p1, s, v2),
                                   argument(gcCache, t2, p2, s, v3));
            break;
        }
        default:
        {
            cout << "ERROR: too many arguments to qt.invoke" << endl;
            result = false;
        }
        }

        if (!result)
        {
            cout << "ERROR: MuQt invoke failed for "
                 << method.methodSignature().constData() << endl;
        }

        NODE_RETURN(result);
    }

#include <qtTypeDefinitions.cpp>

    void qtModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        //
        //  Register types so that SignalSpy works (see qtUtils.h for DECLARE
        //  macros)
        //

        qRegisterMetaType<QTreeWidgetItem*>();
        qRegisterMetaType<QListWidgetItem*>();
        qRegisterMetaType<QTableWidgetItem*>();
        qRegisterMetaType<QStandardItem*>();
        qRegisterMetaType<QAction*>();
        qRegisterMetaType<QModelIndex>();
        qRegisterMetaType<QPixmap>();
        qRegisterMetaType<QItemSelection>();
        qRegisterMetaType<Qt::DockWidgetArea>();
        qRegisterMetaType<Qt::DockWidgetAreas>();
        qRegisterMetaType<QDockWidget::DockWidgetFeatures>();

        // cout << "QT LOAD START" << endl;

        MuLangContext* c = static_cast<MuLangContext*>(context());
        Module* global = globalModule();

        //
        //  This OpaqueType is the QObject reference
        //

        addSymbol(new OpaqueType(c, "NativeObject"));

        //
        //  Add all of the classes from the generated function
        //

        QWidgetType* widgetType = addAllQTSymbols(c, this);

        //
        //  Create the qt module and add all auto generated symbols from
        //  namespace Qt
        //

        Module* qt = new Module(c, "Qt"); // namespace Qt

        qt->addSymbols(
            new Alias(c, "AlignmentFlag", "int"),
            new SymbolicConstant(c, "AlignLeft", "int", Value(int(0x0001))),
            new SymbolicConstant(c, "AlignRight", "int", Value(int(0x0002))),
            new SymbolicConstant(c, "AlignHCenter", "int", Value(int(0x0004))),
            new SymbolicConstant(c, "AlignJustify", "int", Value(int(0x0008))),
            new SymbolicConstant(c, "AlignTop", "int", Value(int(0x0020))),
            new SymbolicConstant(c, "AlignBottom", "int", Value(int(0x0040))),
            new SymbolicConstant(c, "AlignVCenter", "int", Value(int(0x0080))),
            new SymbolicConstant(
                c, "AlignCenter", "int",
                Value(int(Qt::AlignVCenter | Qt::AlignHCenter))),
            new SymbolicConstant(c, "AlignAbsolute", "int", Value(int(0x0010))),
            new SymbolicConstant(c, "AlignLeading", "int",
                                 Value(int(Qt::AlignLeft))),
            new SymbolicConstant(c, "AlignTrailing", "int",
                                 Value(int(Qt::AlignRight))),
            new SymbolicConstant(
                c, "AlignHorizontal_Mask", "int",
                Value(int(Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter
                          | Qt::AlignJustify | Qt::AlignAbsolute))),
            new SymbolicConstant(
                c, "AlignVertical_Mask", "int",
                Value(int(Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter))),
            new Alias(c, "AnchorAttribute", "int"),
            new SymbolicConstant(c, "AnchorName", "int", Value(int(0))),
            new SymbolicConstant(c, "AnchorHref", "int", Value(int(1))),
            new Alias(c, "ApplicationAttribute", "int"),
            new SymbolicConstant(c, "AA_ImmediateWidgetCreation", "int",
                                 Value(int(0))),
            new SymbolicConstant(c, "AA_MSWindowsUseDirect3DByDefault", "int",
                                 Value(int(1))),
            new SymbolicConstant(c, "AA_DontShowIconsInMenus", "int",
                                 Value(int(2))),
            new SymbolicConstant(c, "AA_NativeWindows", "int", Value(int(3))),
            new SymbolicConstant(c, "AA_DontCreateNativeWidgetSiblings", "int",
                                 Value(int(4))),
            new SymbolicConstant(c, "AA_MacPluginApplication", "int",
                                 Value(int(5))),
            new Alias(c, "ArrowType", "int"),
            new SymbolicConstant(c, "NoArrow", "int", Value(int(0))),
            new SymbolicConstant(c, "UpArrow", "int", Value(int(1))),
            new SymbolicConstant(c, "DownArrow", "int", Value(int(2))),
            new SymbolicConstant(c, "LeftArrow", "int", Value(int(3))),
            new SymbolicConstant(c, "RightArrow", "int", Value(int(4))),
            new Alias(c, "AspectRatioMode", "int"),
            new SymbolicConstant(c, "IgnoreAspectRatio", "int", Value(int(0))),
            new SymbolicConstant(c, "KeepAspectRatio", "int", Value(int(1))),
            new SymbolicConstant(c, "KeepAspectRatioByExpanding", "int",
                                 Value(int(2))),
            new Alias(c, "Axis", "int"),
            new SymbolicConstant(c, "XAxis", "int", Value(int(0))),
            new SymbolicConstant(c, "YAxis", "int", Value(int(1))),
            new SymbolicConstant(c, "ZAxis", "int", Value(int(2))),
            new Alias(c, "BGMode", "int"),
            new SymbolicConstant(c, "TransparentMode", "int", Value(int(0))),
            new SymbolicConstant(c, "OpaqueMode", "int", Value(int(1))),
            new Alias(c, "BrushStyle", "int"),
            new SymbolicConstant(c, "NoBrush", "int", Value(int(0))),
            new SymbolicConstant(c, "SolidPattern", "int", Value(int(1))),
            new SymbolicConstant(c, "Dense1Pattern", "int", Value(int(2))),
            new SymbolicConstant(c, "Dense2Pattern", "int", Value(int(3))),
            new SymbolicConstant(c, "Dense3Pattern", "int", Value(int(4))),
            new SymbolicConstant(c, "Dense4Pattern", "int", Value(int(5))),
            new SymbolicConstant(c, "Dense5Pattern", "int", Value(int(6))),
            new SymbolicConstant(c, "Dense6Pattern", "int", Value(int(7))),
            new SymbolicConstant(c, "Dense7Pattern", "int", Value(int(8))),
            new SymbolicConstant(c, "HorPattern", "int", Value(int(9))),
            new SymbolicConstant(c, "VerPattern", "int", Value(int(10))),
            new SymbolicConstant(c, "CrossPattern", "int", Value(int(11))),
            new SymbolicConstant(c, "BDiagPattern", "int", Value(int(12))),
            new SymbolicConstant(c, "FDiagPattern", "int", Value(int(13))),
            new SymbolicConstant(c, "DiagCrossPattern", "int", Value(int(14))),
            new SymbolicConstant(c, "LinearGradientPattern", "int",
                                 Value(int(15))),
            new SymbolicConstant(c, "ConicalGradientPattern", "int",
                                 Value(int(17))),
            new SymbolicConstant(c, "RadialGradientPattern", "int",
                                 Value(int(16))),
            new SymbolicConstant(c, "TexturePattern", "int", Value(int(24))),
            new Alias(c, "CaseSensitivity", "int"),
            new SymbolicConstant(c, "CaseInsensitive", "int", Value(int(0))),
            new SymbolicConstant(c, "CaseSensitive", "int", Value(int(1))),
            new Alias(c, "CheckState", "int"),
            new SymbolicConstant(c, "Unchecked", "int", Value(int(0))),
            new SymbolicConstant(c, "PartiallyChecked", "int", Value(int(1))),
            new SymbolicConstant(c, "Checked", "int", Value(int(2))),
            new Alias(c, "ClipOperation", "int"),
            new SymbolicConstant(c, "NoClip", "int", Value(int(0))),
            new SymbolicConstant(c, "ReplaceClip", "int", Value(int(1))),
            new SymbolicConstant(c, "IntersectClip", "int", Value(int(2))),
            new SymbolicConstant(c, "UniteClip", "int", Value(int(3))),
            new Alias(c, "ConnectionType", "int"),
            new SymbolicConstant(c, "DirectConnection", "int", Value(int(1))),
            new SymbolicConstant(c, "QueuedConnection", "int", Value(int(2))),
            new SymbolicConstant(c, "BlockingQueuedConnection", "int",
                                 Value(int(4))),
            new SymbolicConstant(c, "UniqueConnection", "int", Value(int(5))),
            new SymbolicConstant(c, "AutoConnection", "int", Value(int(0))),
            new Alias(c, "ContextMenuPolicy", "int"),
            new SymbolicConstant(c, "NoContextMenu", "int", Value(int(0))),
            new SymbolicConstant(c, "PreventContextMenu", "int", Value(int(4))),
            new SymbolicConstant(c, "DefaultContextMenu", "int", Value(int(1))),
            new SymbolicConstant(c, "ActionsContextMenu", "int", Value(int(2))),
            new SymbolicConstant(c, "CustomContextMenu", "int", Value(int(3))),
            new Alias(c, "Corner", "int"),
            new SymbolicConstant(c, "TopLeftCorner", "int",
                                 Value(int(0x00000))),
            new SymbolicConstant(c, "TopRightCorner", "int",
                                 Value(int(0x00001))),
            new SymbolicConstant(c, "BottomLeftCorner", "int",
                                 Value(int(0x00002))),
            new SymbolicConstant(c, "BottomRightCorner", "int",
                                 Value(int(0x00003))),
            new Alias(c, "CursorShape", "int"),
            new SymbolicConstant(c, "ArrowCursor", "int", Value(int(0))),
            new SymbolicConstant(c, "UpArrowCursor", "int", Value(int(1))),
            new SymbolicConstant(c, "CrossCursor", "int", Value(int(2))),
            new SymbolicConstant(c, "WaitCursor", "int", Value(int(3))),
            new SymbolicConstant(c, "IBeamCursor", "int", Value(int(4))),
            new SymbolicConstant(c, "SizeVerCursor", "int", Value(int(5))),
            new SymbolicConstant(c, "SizeHorCursor", "int", Value(int(6))),
            new SymbolicConstant(c, "SizeBDiagCursor", "int", Value(int(7))),
            new SymbolicConstant(c, "SizeFDiagCursor", "int", Value(int(8))),
            new SymbolicConstant(c, "SizeAllCursor", "int", Value(int(9))),
            new SymbolicConstant(c, "BlankCursor", "int", Value(int(10))),
            new SymbolicConstant(c, "SplitVCursor", "int", Value(int(11))),
            new SymbolicConstant(c, "SplitHCursor", "int", Value(int(12))),
            new SymbolicConstant(c, "PointingHandCursor", "int",
                                 Value(int(13))),
            new SymbolicConstant(c, "ForbiddenCursor", "int", Value(int(14))),
            new SymbolicConstant(c, "OpenHandCursor", "int", Value(int(17))),
            new SymbolicConstant(c, "ClosedHandCursor", "int", Value(int(18))),
            new SymbolicConstant(c, "WhatsThisCursor", "int", Value(int(15))),
            new SymbolicConstant(c, "BusyCursor", "int", Value(int(16))),
            new SymbolicConstant(c, "BitmapCursor", "int", Value(int(24))),
            new Alias(c, "DateFormat", "int"),
            new SymbolicConstant(c, "TextDate", "int", Value(int(0))),
            new SymbolicConstant(c, "ISODate", "int", Value(int(1))),
            // new SymbolicConstant(c, "SystemLocaleShortDate", "int",
            // Value(int(?))), new SymbolicConstant(c, "SystemLocaleLongDate",
            // "int", Value(int(?))), new SymbolicConstant(c,
            // "DefaultLocaleShortDate", "int", Value(int(?))), new
            // SymbolicConstant(c, "DefaultLocaleLongDate", "int",
            // Value(int(?))),
            new SymbolicConstant(c, "SystemLocaleDate", "int", Value(int(2))),
            // new SymbolicConstant(c, "LocaleDate", "int", Value(int(?))),
            new SymbolicConstant(c, "LocalDate", "int",
                                 Value(int(Qt::SystemLocaleDate))),
            new Alias(c, "DayOfWeek", "int"),
            new SymbolicConstant(c, "Monday", "int", Value(int(1))),
            new SymbolicConstant(c, "Tuesday", "int", Value(int(2))),
            new SymbolicConstant(c, "Wednesday", "int", Value(int(3))),
            new SymbolicConstant(c, "Thursday", "int", Value(int(4))),
            new SymbolicConstant(c, "Friday", "int", Value(int(5))),
            new SymbolicConstant(c, "Saturday", "int", Value(int(6))),
            new SymbolicConstant(c, "Sunday", "int", Value(int(7))),
            new Alias(c, "DockWidgetArea", "int"),
            new SymbolicConstant(c, "LeftDockWidgetArea", "int",
                                 Value(int(0x1))),
            new SymbolicConstant(c, "RightDockWidgetArea", "int",
                                 Value(int(0x2))),
            new SymbolicConstant(c, "TopDockWidgetArea", "int",
                                 Value(int(0x4))),
            new SymbolicConstant(c, "BottomDockWidgetArea", "int",
                                 Value(int(0x8))),
            new SymbolicConstant(c, "AllDockWidgetAreas", "int",
                                 Value(int(Qt::DockWidgetArea_Mask))),
            new SymbolicConstant(c, "NoDockWidgetArea", "int", Value(int(0))),
            new Alias(c, "DropAction", "int"),
            new SymbolicConstant(c, "CopyAction", "int", Value(int(0x1))),
            new SymbolicConstant(c, "MoveAction", "int", Value(int(0x2))),
            new SymbolicConstant(c, "LinkAction", "int", Value(int(0x4))),
            new SymbolicConstant(c, "ActionMask", "int", Value(int(0xff))),
            new SymbolicConstant(c, "IgnoreAction", "int", Value(int(0x0))),
            new SymbolicConstant(c, "TargetMoveAction", "int",
                                 Value(int(0x8002))),
            new Alias(c, "EventPriority", "int"),
            new SymbolicConstant(c, "HighEventPriority", "int", Value(int(1))),
            new SymbolicConstant(c, "NormalEventPriority", "int",
                                 Value(int(0))),
            new SymbolicConstant(c, "LowEventPriority", "int", Value(int(-1))),
            new Alias(c, "FillRule", "int"),
            new SymbolicConstant(c, "OddEvenFill", "int", Value(int(0))),
            new SymbolicConstant(c, "WindingFill", "int", Value(int(1))),
            new Alias(c, "FocusPolicy", "int"),
            new SymbolicConstant(c, "TabFocus", "int", Value(int(0x1))),
            new SymbolicConstant(c, "ClickFocus", "int", Value(int(0x2))),
            new SymbolicConstant(
                c, "StrongFocus", "int",
                Value(int(Qt::TabFocus | Qt::ClickFocus | 0x8))),
            new SymbolicConstant(c, "WheelFocus", "int",
                                 Value(int(Qt::StrongFocus | 0x4))),
            new SymbolicConstant(c, "NoFocus", "int", Value(int(0))),
            new Alias(c, "FocusReason", "int"),
            new SymbolicConstant(c, "MouseFocusReason", "int", Value(int(0))),
            new SymbolicConstant(c, "TabFocusReason", "int", Value(int(1))),
            new SymbolicConstant(c, "BacktabFocusReason", "int", Value(int(2))),
            new SymbolicConstant(c, "ActiveWindowFocusReason", "int",
                                 Value(int(3))),
            new SymbolicConstant(c, "PopupFocusReason", "int", Value(int(4))),
            new SymbolicConstant(c, "ShortcutFocusReason", "int",
                                 Value(int(5))),
            new SymbolicConstant(c, "MenuBarFocusReason", "int", Value(int(6))),
            new SymbolicConstant(c, "OtherFocusReason", "int", Value(int(7))),
            new Alias(c, "GlobalColor", "int"),
            new SymbolicConstant(c, "white", "int", Value(int(3))),
            new SymbolicConstant(c, "black", "int", Value(int(2))),
            new SymbolicConstant(c, "red", "int", Value(int(7))),
            new SymbolicConstant(c, "darkRed", "int", Value(int(13))),
            new SymbolicConstant(c, "green", "int", Value(int(8))),
            new SymbolicConstant(c, "darkGreen", "int", Value(int(14))),
            new SymbolicConstant(c, "blue", "int", Value(int(9))),
            new SymbolicConstant(c, "darkBlue", "int", Value(int(15))),
            new SymbolicConstant(c, "cyan", "int", Value(int(10))),
            new SymbolicConstant(c, "darkCyan", "int", Value(int(16))),
            new SymbolicConstant(c, "magenta", "int", Value(int(11))),
            new SymbolicConstant(c, "darkMagenta", "int", Value(int(17))),
            new SymbolicConstant(c, "yellow", "int", Value(int(12))),
            new SymbolicConstant(c, "darkYellow", "int", Value(int(18))),
            new SymbolicConstant(c, "gray", "int", Value(int(5))),
            new SymbolicConstant(c, "darkGray", "int", Value(int(4))),
            new SymbolicConstant(c, "lightGray", "int", Value(int(6))),
            new SymbolicConstant(c, "transparent", "int", Value(int(19))),
            new SymbolicConstant(c, "color0", "int", Value(int(0))),
            new SymbolicConstant(c, "color1", "int", Value(int(1))),
            new Alias(c, "HitTestAccuracy", "int"),
            new SymbolicConstant(c, "ExactHit", "int", Value(int(0))),
            new SymbolicConstant(c, "FuzzyHit", "int", Value(int(1))),
            new Alias(c, "ImageConversionFlag", "int"),
            new SymbolicConstant(c, "AutoColor", "int", Value(int(0x00000000))),
            new SymbolicConstant(c, "ColorOnly", "int", Value(int(0x00000003))),
            new SymbolicConstant(c, "MonoOnly", "int", Value(int(0x00000002))),
            new SymbolicConstant(c, "DiffuseDither", "int",
                                 Value(int(0x00000000))),
            new SymbolicConstant(c, "OrderedDither", "int",
                                 Value(int(0x00000010))),
            new SymbolicConstant(c, "ThresholdDither", "int",
                                 Value(int(0x00000020))),
            new SymbolicConstant(c, "ThresholdAlphaDither", "int",
                                 Value(int(0x00000000))),
            new SymbolicConstant(c, "OrderedAlphaDither", "int",
                                 Value(int(0x00000004))),
            new SymbolicConstant(c, "DiffuseAlphaDither", "int",
                                 Value(int(0x00000008))),
            new SymbolicConstant(c, "PreferDither", "int",
                                 Value(int(0x00000040))),
            new SymbolicConstant(c, "AvoidDither", "int",
                                 Value(int(0x00000080))),
            new Alias(c, "InputMethodQuery", "int"),
            new SymbolicConstant(c, "ImMicroFocus", "int", Value(int(0))),
            new SymbolicConstant(c, "ImFont", "int", Value(int(1))),
            new SymbolicConstant(c, "ImCursorPosition", "int", Value(int(2))),
            new SymbolicConstant(c, "ImSurroundingText", "int", Value(int(3))),
            new SymbolicConstant(c, "ImCurrentSelection", "int", Value(int(4))),
            new Alias(c, "ItemDataRole", "int"),
            new SymbolicConstant(c, "DisplayRole", "int", Value(int(0))),
            new SymbolicConstant(c, "DecorationRole", "int", Value(int(1))),
            new SymbolicConstant(c, "EditRole", "int", Value(int(2))),
            new SymbolicConstant(c, "ToolTipRole", "int", Value(int(3))),
            new SymbolicConstant(c, "StatusTipRole", "int", Value(int(4))),
            new SymbolicConstant(c, "WhatsThisRole", "int", Value(int(5))),
            new SymbolicConstant(c, "SizeHintRole", "int", Value(int(13))),
            new SymbolicConstant(c, "FontRole", "int", Value(int(6))),
            new SymbolicConstant(c, "TextAlignmentRole", "int", Value(int(7))),
            new SymbolicConstant(c, "BackgroundRole", "int", Value(int(8))),
            new SymbolicConstant(c, "BackgroundColorRole", "int",
                                 Value(int(8))),
            new SymbolicConstant(c, "ForegroundRole", "int", Value(int(9))),
            new SymbolicConstant(c, "TextColorRole", "int", Value(int(9))),
            new SymbolicConstant(c, "CheckStateRole", "int", Value(int(10))),
            new SymbolicConstant(c, "AccessibleTextRole", "int",
                                 Value(int(11))),
            new SymbolicConstant(c, "AccessibleDescriptionRole", "int",
                                 Value(int(12))),
            new SymbolicConstant(c, "UserRole", "int", Value(int(256))),
            new Alias(c, "ItemFlag", "int"),
            new SymbolicConstant(c, "NoItemFlags", "int", Value(int(0))),
            new SymbolicConstant(c, "ItemIsSelectable", "int", Value(int(1))),
            new SymbolicConstant(c, "ItemIsEditable", "int", Value(int(2))),
            new SymbolicConstant(c, "ItemIsDragEnabled", "int", Value(int(4))),
            new SymbolicConstant(c, "ItemIsDropEnabled", "int", Value(int(8))),
            new SymbolicConstant(c, "ItemIsUserCheckable", "int",
                                 Value(int(16))),
            new SymbolicConstant(c, "ItemIsEnabled", "int", Value(int(32))),
            new SymbolicConstant(c, "ItemIsTristate", "int", Value(int(64))),
            new Alias(c, "ItemSelectionMode", "int"),
            new SymbolicConstant(c, "ContainsItemShape", "int",
                                 Value(int(0x0))),
            new SymbolicConstant(c, "IntersectsItemShape", "int",
                                 Value(int(0x1))),
            new SymbolicConstant(c, "ContainsItemBoundingRect", "int",
                                 Value(int(0x2))),
            new SymbolicConstant(c, "IntersectsItemBoundingRect", "int",
                                 Value(int(0x3))),
            new Alias(c, "Key", "int"),
            new SymbolicConstant(c, "Key_Escape", "int",
                                 Value(int(0x01000000))),
            new SymbolicConstant(c, "Key_Tab", "int", Value(int(0x01000001))),
            new SymbolicConstant(c, "Key_Backtab", "int",
                                 Value(int(0x01000002))),
            new SymbolicConstant(c, "Key_Backspace", "int",
                                 Value(int(0x01000003))),
            new SymbolicConstant(c, "Key_Return", "int",
                                 Value(int(0x01000004))),
            new SymbolicConstant(c, "Key_Enter", "int", Value(int(0x01000005))),
            new SymbolicConstant(c, "Key_Insert", "int",
                                 Value(int(0x01000006))),
            new SymbolicConstant(c, "Key_Delete", "int",
                                 Value(int(0x01000007))),
            new SymbolicConstant(c, "Key_Pause", "int", Value(int(0x01000008))),
            new SymbolicConstant(c, "Key_Print", "int", Value(int(0x01000009))),
            new SymbolicConstant(c, "Key_SysReq", "int",
                                 Value(int(0x0100000a))),
            new SymbolicConstant(c, "Key_Clear", "int", Value(int(0x0100000b))),
            new SymbolicConstant(c, "Key_Home", "int", Value(int(0x01000010))),
            new SymbolicConstant(c, "Key_End", "int", Value(int(0x01000011))),
            new SymbolicConstant(c, "Key_Left", "int", Value(int(0x01000012))),
            new SymbolicConstant(c, "Key_Up", "int", Value(int(0x01000013))),
            new SymbolicConstant(c, "Key_Right", "int", Value(int(0x01000014))),
            new SymbolicConstant(c, "Key_Down", "int", Value(int(0x01000015))),
            new SymbolicConstant(c, "Key_PageUp", "int",
                                 Value(int(0x01000016))),
            new SymbolicConstant(c, "Key_PageDown", "int",
                                 Value(int(0x01000017))),
            new SymbolicConstant(c, "Key_Shift", "int", Value(int(0x01000020))),
            new SymbolicConstant(c, "Key_Control", "int",
                                 Value(int(0x01000021))),
            new SymbolicConstant(c, "Key_Meta", "int", Value(int(0x01000022))),
            new SymbolicConstant(c, "Key_Alt", "int", Value(int(0x01000023))),
            new SymbolicConstant(c, "Key_AltGr", "int", Value(int(0x01001103))),
            new SymbolicConstant(c, "Key_CapsLock", "int",
                                 Value(int(0x01000024))),
            new SymbolicConstant(c, "Key_NumLock", "int",
                                 Value(int(0x01000025))),
            new SymbolicConstant(c, "Key_ScrollLock", "int",
                                 Value(int(0x01000026))),
            new SymbolicConstant(c, "Key_F1", "int", Value(int(0x01000030))),
            new SymbolicConstant(c, "Key_F2", "int", Value(int(0x01000031))),
            new SymbolicConstant(c, "Key_F3", "int", Value(int(0x01000032))),
            new SymbolicConstant(c, "Key_F4", "int", Value(int(0x01000033))),
            new SymbolicConstant(c, "Key_F5", "int", Value(int(0x01000034))),
            new SymbolicConstant(c, "Key_F6", "int", Value(int(0x01000035))),
            new SymbolicConstant(c, "Key_F7", "int", Value(int(0x01000036))),
            new SymbolicConstant(c, "Key_F8", "int", Value(int(0x01000037))),
            new SymbolicConstant(c, "Key_F9", "int", Value(int(0x01000038))),
            new SymbolicConstant(c, "Key_F10", "int", Value(int(0x01000039))),
            new SymbolicConstant(c, "Key_F11", "int", Value(int(0x0100003a))),
            new SymbolicConstant(c, "Key_F12", "int", Value(int(0x0100003b))),
            new SymbolicConstant(c, "Key_F13", "int", Value(int(0x0100003c))),
            new SymbolicConstant(c, "Key_F14", "int", Value(int(0x0100003d))),
            new SymbolicConstant(c, "Key_F15", "int", Value(int(0x0100003e))),
            new SymbolicConstant(c, "Key_F16", "int", Value(int(0x0100003f))),
            new SymbolicConstant(c, "Key_F17", "int", Value(int(0x01000040))),
            new SymbolicConstant(c, "Key_F18", "int", Value(int(0x01000041))),
            new SymbolicConstant(c, "Key_F19", "int", Value(int(0x01000042))),
            new SymbolicConstant(c, "Key_F20", "int", Value(int(0x01000043))),
            new SymbolicConstant(c, "Key_F21", "int", Value(int(0x01000044))),
            new SymbolicConstant(c, "Key_F22", "int", Value(int(0x01000045))),
            new SymbolicConstant(c, "Key_F23", "int", Value(int(0x01000046))),
            new SymbolicConstant(c, "Key_F24", "int", Value(int(0x01000047))),
            new SymbolicConstant(c, "Key_F25", "int", Value(int(0x01000048))),
            new SymbolicConstant(c, "Key_F26", "int", Value(int(0x01000049))),
            new SymbolicConstant(c, "Key_F27", "int", Value(int(0x0100004a))),
            new SymbolicConstant(c, "Key_F28", "int", Value(int(0x0100004b))),
            new SymbolicConstant(c, "Key_F29", "int", Value(int(0x0100004c))),
            new SymbolicConstant(c, "Key_F30", "int", Value(int(0x0100004d))),
            new SymbolicConstant(c, "Key_F31", "int", Value(int(0x0100004e))),
            new SymbolicConstant(c, "Key_F32", "int", Value(int(0x0100004f))),
            new SymbolicConstant(c, "Key_F33", "int", Value(int(0x01000050))),
            new SymbolicConstant(c, "Key_F34", "int", Value(int(0x01000051))),
            new SymbolicConstant(c, "Key_F35", "int", Value(int(0x01000052))),
            new SymbolicConstant(c, "Key_Super_L", "int",
                                 Value(int(0x01000053))),
            new SymbolicConstant(c, "Key_Super_R", "int",
                                 Value(int(0x01000054))),
            new SymbolicConstant(c, "Key_Menu", "int", Value(int(0x01000055))),
            new SymbolicConstant(c, "Key_Hyper_L", "int",
                                 Value(int(0x01000056))),
            new SymbolicConstant(c, "Key_Hyper_R", "int",
                                 Value(int(0x01000057))),
            new SymbolicConstant(c, "Key_Help", "int", Value(int(0x01000058))),
            new SymbolicConstant(c, "Key_Direction_L", "int",
                                 Value(int(0x01000059))),
            new SymbolicConstant(c, "Key_Direction_R", "int",
                                 Value(int(0x01000060))),
            new SymbolicConstant(c, "Key_Space", "int", Value(int(0x20))),
            new SymbolicConstant(c, "Key_Any", "int",
                                 Value(int(Qt::Key_Space))),
            new SymbolicConstant(c, "Key_Exclam", "int", Value(int(0x21))),
            new SymbolicConstant(c, "Key_QuoteDbl", "int", Value(int(0x22))),
            new SymbolicConstant(c, "Key_NumberSign", "int", Value(int(0x23))),
            new SymbolicConstant(c, "Key_Dollar", "int", Value(int(0x24))),
            new SymbolicConstant(c, "Key_Percent", "int", Value(int(0x25))),
            new SymbolicConstant(c, "Key_Ampersand", "int", Value(int(0x26))),
            new SymbolicConstant(c, "Key_Apostrophe", "int", Value(int(0x27))),
            new SymbolicConstant(c, "Key_ParenLeft", "int", Value(int(0x28))),
            new SymbolicConstant(c, "Key_ParenRight", "int", Value(int(0x29))),
            new SymbolicConstant(c, "Key_Asterisk", "int", Value(int(0x2a))),
            new SymbolicConstant(c, "Key_Plus", "int", Value(int(0x2b))),
            new SymbolicConstant(c, "Key_Comma", "int", Value(int(0x2c))),
            new SymbolicConstant(c, "Key_Minus", "int", Value(int(0x2d))),
            new SymbolicConstant(c, "Key_Period", "int", Value(int(0x2e))),
            new SymbolicConstant(c, "Key_Slash", "int", Value(int(0x2f))),
            new SymbolicConstant(c, "Key_0", "int", Value(int(0x30))),
            new SymbolicConstant(c, "Key_1", "int", Value(int(0x31))),
            new SymbolicConstant(c, "Key_2", "int", Value(int(0x32))),
            new SymbolicConstant(c, "Key_3", "int", Value(int(0x33))),
            new SymbolicConstant(c, "Key_4", "int", Value(int(0x34))),
            new SymbolicConstant(c, "Key_5", "int", Value(int(0x35))),
            new SymbolicConstant(c, "Key_6", "int", Value(int(0x36))),
            new SymbolicConstant(c, "Key_7", "int", Value(int(0x37))),
            new SymbolicConstant(c, "Key_8", "int", Value(int(0x38))),
            new SymbolicConstant(c, "Key_9", "int", Value(int(0x39))),
            new SymbolicConstant(c, "Key_Colon", "int", Value(int(0x3a))),
            new SymbolicConstant(c, "Key_Semicolon", "int", Value(int(0x3b))),
            new SymbolicConstant(c, "Key_Less", "int", Value(int(0x3c))),
            new SymbolicConstant(c, "Key_Equal", "int", Value(int(0x3d))),
            new SymbolicConstant(c, "Key_Greater", "int", Value(int(0x3e))),
            new SymbolicConstant(c, "Key_Question", "int", Value(int(0x3f))),
            new SymbolicConstant(c, "Key_At", "int", Value(int(0x40))),
            new SymbolicConstant(c, "Key_A", "int", Value(int(0x41))),
            new SymbolicConstant(c, "Key_B", "int", Value(int(0x42))),
            new SymbolicConstant(c, "Key_C", "int", Value(int(0x43))),
            new SymbolicConstant(c, "Key_D", "int", Value(int(0x44))),
            new SymbolicConstant(c, "Key_E", "int", Value(int(0x45))),
            new SymbolicConstant(c, "Key_F", "int", Value(int(0x46))),
            new SymbolicConstant(c, "Key_G", "int", Value(int(0x47))),
            new SymbolicConstant(c, "Key_H", "int", Value(int(0x48))),
            new SymbolicConstant(c, "Key_I", "int", Value(int(0x49))),
            new SymbolicConstant(c, "Key_J", "int", Value(int(0x4a))),
            new SymbolicConstant(c, "Key_K", "int", Value(int(0x4b))),
            new SymbolicConstant(c, "Key_L", "int", Value(int(0x4c))),
            new SymbolicConstant(c, "Key_M", "int", Value(int(0x4d))),
            new SymbolicConstant(c, "Key_N", "int", Value(int(0x4e))),
            new SymbolicConstant(c, "Key_O", "int", Value(int(0x4f))),
            new SymbolicConstant(c, "Key_P", "int", Value(int(0x50))),
            new SymbolicConstant(c, "Key_Q", "int", Value(int(0x51))),
            new SymbolicConstant(c, "Key_R", "int", Value(int(0x52))),
            new SymbolicConstant(c, "Key_S", "int", Value(int(0x53))),
            new SymbolicConstant(c, "Key_T", "int", Value(int(0x54))),
            new SymbolicConstant(c, "Key_U", "int", Value(int(0x55))),
            new SymbolicConstant(c, "Key_V", "int", Value(int(0x56))),
            new SymbolicConstant(c, "Key_W", "int", Value(int(0x57))),
            new SymbolicConstant(c, "Key_X", "int", Value(int(0x58))),
            new SymbolicConstant(c, "Key_Y", "int", Value(int(0x59))),
            new SymbolicConstant(c, "Key_Z", "int", Value(int(0x5a))),
            new SymbolicConstant(c, "Key_BracketLeft", "int", Value(int(0x5b))),
            new SymbolicConstant(c, "Key_Backslash", "int", Value(int(0x5c))),
            new SymbolicConstant(c, "Key_BracketRight", "int",
                                 Value(int(0x5d))),
            new SymbolicConstant(c, "Key_AsciiCircum", "int", Value(int(0x5e))),
            new SymbolicConstant(c, "Key_Underscore", "int", Value(int(0x5f))),
            new SymbolicConstant(c, "Key_QuoteLeft", "int", Value(int(0x60))),
            new SymbolicConstant(c, "Key_BraceLeft", "int", Value(int(0x7b))),
            new SymbolicConstant(c, "Key_Bar", "int", Value(int(0x7c))),
            new SymbolicConstant(c, "Key_BraceRight", "int", Value(int(0x7d))),
            new SymbolicConstant(c, "Key_AsciiTilde", "int", Value(int(0x7e))),
            new SymbolicConstant(c, "Key_nobreakspace", "int",
                                 Value(int(0x0a0))),
            new SymbolicConstant(c, "Key_exclamdown", "int", Value(int(0x0a1))),
            new SymbolicConstant(c, "Key_cent", "int", Value(int(0x0a2))),
            new SymbolicConstant(c, "Key_sterling", "int", Value(int(0x0a3))),
            new SymbolicConstant(c, "Key_currency", "int", Value(int(0x0a4))),
            new SymbolicConstant(c, "Key_yen", "int", Value(int(0x0a5))),
            new SymbolicConstant(c, "Key_brokenbar", "int", Value(int(0x0a6))),
            new SymbolicConstant(c, "Key_section", "int", Value(int(0x0a7))),
            new SymbolicConstant(c, "Key_diaeresis", "int", Value(int(0x0a8))),
            new SymbolicConstant(c, "Key_copyright", "int", Value(int(0x0a9))),
            new SymbolicConstant(c, "Key_ordfeminine", "int",
                                 Value(int(0x0aa))),
            new SymbolicConstant(c, "Key_guillemotleft", "int",
                                 Value(int(0x0ab))),
            new SymbolicConstant(c, "Key_notsign", "int", Value(int(0x0ac))),
            new SymbolicConstant(c, "Key_hyphen", "int", Value(int(0x0ad))),
            new SymbolicConstant(c, "Key_registered", "int", Value(int(0x0ae))),
            new SymbolicConstant(c, "Key_macron", "int", Value(int(0x0af))),
            new SymbolicConstant(c, "Key_degree", "int", Value(int(0x0b0))),
            new SymbolicConstant(c, "Key_plusminus", "int", Value(int(0x0b1))),
            new SymbolicConstant(c, "Key_twosuperior", "int",
                                 Value(int(0x0b2))),
            new SymbolicConstant(c, "Key_threesuperior", "int",
                                 Value(int(0x0b3))),
            new SymbolicConstant(c, "Key_acute", "int", Value(int(0x0b4))),
            new SymbolicConstant(c, "Key_mu", "int", Value(int(0x0b5))),
            new SymbolicConstant(c, "Key_paragraph", "int", Value(int(0x0b6))),
            new SymbolicConstant(c, "Key_periodcentered", "int",
                                 Value(int(0x0b7))),
            new SymbolicConstant(c, "Key_cedilla", "int", Value(int(0x0b8))),
            new SymbolicConstant(c, "Key_onesuperior", "int",
                                 Value(int(0x0b9))),
            new SymbolicConstant(c, "Key_masculine", "int", Value(int(0x0ba))),
            new SymbolicConstant(c, "Key_guillemotright", "int",
                                 Value(int(0x0bb))),
            new SymbolicConstant(c, "Key_onequarter", "int", Value(int(0x0bc))),
            new SymbolicConstant(c, "Key_onehalf", "int", Value(int(0x0bd))),
            new SymbolicConstant(c, "Key_threequarters", "int",
                                 Value(int(0x0be))),
            new SymbolicConstant(c, "Key_questiondown", "int",
                                 Value(int(0x0bf))),
            new SymbolicConstant(c, "Key_Agrave", "int", Value(int(0x0c0))),
            new SymbolicConstant(c, "Key_Aacute", "int", Value(int(0x0c1))),
            new SymbolicConstant(c, "Key_Acircumflex", "int",
                                 Value(int(0x0c2))),
            new SymbolicConstant(c, "Key_Atilde", "int", Value(int(0x0c3))),
            new SymbolicConstant(c, "Key_Adiaeresis", "int", Value(int(0x0c4))),
            new SymbolicConstant(c, "Key_Aring", "int", Value(int(0x0c5))),
            new SymbolicConstant(c, "Key_AE", "int", Value(int(0x0c6))),
            new SymbolicConstant(c, "Key_Ccedilla", "int", Value(int(0x0c7))),
            new SymbolicConstant(c, "Key_Egrave", "int", Value(int(0x0c8))),
            new SymbolicConstant(c, "Key_Eacute", "int", Value(int(0x0c9))),
            new SymbolicConstant(c, "Key_Ecircumflex", "int",
                                 Value(int(0x0ca))),
            new SymbolicConstant(c, "Key_Ediaeresis", "int", Value(int(0x0cb))),
            new SymbolicConstant(c, "Key_Igrave", "int", Value(int(0x0cc))),
            new SymbolicConstant(c, "Key_Iacute", "int", Value(int(0x0cd))),
            new SymbolicConstant(c, "Key_Icircumflex", "int",
                                 Value(int(0x0ce))),
            new SymbolicConstant(c, "Key_Idiaeresis", "int", Value(int(0x0cf))),
            new SymbolicConstant(c, "Key_ETH", "int", Value(int(0x0d0))),
            new SymbolicConstant(c, "Key_Ntilde", "int", Value(int(0x0d1))),
            new SymbolicConstant(c, "Key_Ograve", "int", Value(int(0x0d2))),
            new SymbolicConstant(c, "Key_Oacute", "int", Value(int(0x0d3))),
            new SymbolicConstant(c, "Key_Ocircumflex", "int",
                                 Value(int(0x0d4))),
            new SymbolicConstant(c, "Key_Otilde", "int", Value(int(0x0d5))),
            new SymbolicConstant(c, "Key_Odiaeresis", "int", Value(int(0x0d6))),
            new SymbolicConstant(c, "Key_multiply", "int", Value(int(0x0d7))),
            new SymbolicConstant(c, "Key_Ooblique", "int", Value(int(0x0d8))),
            new SymbolicConstant(c, "Key_Ugrave", "int", Value(int(0x0d9))),
            new SymbolicConstant(c, "Key_Uacute", "int", Value(int(0x0da))),
            new SymbolicConstant(c, "Key_Ucircumflex", "int",
                                 Value(int(0x0db))),
            new SymbolicConstant(c, "Key_Udiaeresis", "int", Value(int(0x0dc))),
            new SymbolicConstant(c, "Key_Yacute", "int", Value(int(0x0dd))),
            new SymbolicConstant(c, "Key_THORN", "int", Value(int(0x0de))),
            new SymbolicConstant(c, "Key_ssharp", "int", Value(int(0x0df))),
            new SymbolicConstant(c, "Key_division", "int", Value(int(0x0f7))),
            new SymbolicConstant(c, "Key_ydiaeresis", "int", Value(int(0x0ff))),
            new SymbolicConstant(c, "Key_Multi_key", "int",
                                 Value(int(0x01001120))),
            new SymbolicConstant(c, "Key_Codeinput", "int",
                                 Value(int(0x01001137))),
            new SymbolicConstant(c, "Key_SingleCandidate", "int",
                                 Value(int(0x0100113c))),
            new SymbolicConstant(c, "Key_MultipleCandidate", "int",
                                 Value(int(0x0100113d))),
            new SymbolicConstant(c, "Key_PreviousCandidate", "int",
                                 Value(int(0x0100113e))),
            new SymbolicConstant(c, "Key_Mode_switch", "int",
                                 Value(int(0x0100117e))),
            new SymbolicConstant(c, "Key_Kanji", "int", Value(int(0x01001121))),
            new SymbolicConstant(c, "Key_Muhenkan", "int",
                                 Value(int(0x01001122))),
            new SymbolicConstant(c, "Key_Henkan", "int",
                                 Value(int(0x01001123))),
            new SymbolicConstant(c, "Key_Romaji", "int",
                                 Value(int(0x01001124))),
            new SymbolicConstant(c, "Key_Hiragana", "int",
                                 Value(int(0x01001125))),
            new SymbolicConstant(c, "Key_Katakana", "int",
                                 Value(int(0x01001126))),
            new SymbolicConstant(c, "Key_Hiragana_Katakana", "int",
                                 Value(int(0x01001127))),
            new SymbolicConstant(c, "Key_Zenkaku", "int",
                                 Value(int(0x01001128))),
            new SymbolicConstant(c, "Key_Hankaku", "int",
                                 Value(int(0x01001129))),
            new SymbolicConstant(c, "Key_Zenkaku_Hankaku", "int",
                                 Value(int(0x0100112a))),
            new SymbolicConstant(c, "Key_Touroku", "int",
                                 Value(int(0x0100112b))),
            new SymbolicConstant(c, "Key_Massyo", "int",
                                 Value(int(0x0100112c))),
            new SymbolicConstant(c, "Key_Kana_Lock", "int",
                                 Value(int(0x0100112d))),
            new SymbolicConstant(c, "Key_Kana_Shift", "int",
                                 Value(int(0x0100112e))),
            new SymbolicConstant(c, "Key_Eisu_Shift", "int",
                                 Value(int(0x0100112f))),
            new SymbolicConstant(c, "Key_Eisu_toggle", "int",
                                 Value(int(0x01001130))),
            new SymbolicConstant(c, "Key_Hangul", "int",
                                 Value(int(0x01001131))),
            new SymbolicConstant(c, "Key_Hangul_Start", "int",
                                 Value(int(0x01001132))),
            new SymbolicConstant(c, "Key_Hangul_End", "int",
                                 Value(int(0x01001133))),
            new SymbolicConstant(c, "Key_Hangul_Hanja", "int",
                                 Value(int(0x01001134))),
            new SymbolicConstant(c, "Key_Hangul_Jamo", "int",
                                 Value(int(0x01001135))),
            new SymbolicConstant(c, "Key_Hangul_Romaja", "int",
                                 Value(int(0x01001136))),
            new SymbolicConstant(c, "Key_Hangul_Jeonja", "int",
                                 Value(int(0x01001138))),
            new SymbolicConstant(c, "Key_Hangul_Banja", "int",
                                 Value(int(0x01001139))),
            new SymbolicConstant(c, "Key_Hangul_PreHanja", "int",
                                 Value(int(0x0100113a))),
            new SymbolicConstant(c, "Key_Hangul_PostHanja", "int",
                                 Value(int(0x0100113b))),
            new SymbolicConstant(c, "Key_Hangul_Special", "int",
                                 Value(int(0x0100113f))),
            new SymbolicConstant(c, "Key_Dead_Grave", "int",
                                 Value(int(0x01001250))),
            new SymbolicConstant(c, "Key_Dead_Acute", "int",
                                 Value(int(0x01001251))),
            new SymbolicConstant(c, "Key_Dead_Circumflex", "int",
                                 Value(int(0x01001252))),
            new SymbolicConstant(c, "Key_Dead_Tilde", "int",
                                 Value(int(0x01001253))),
            new SymbolicConstant(c, "Key_Dead_Macron", "int",
                                 Value(int(0x01001254))),
            new SymbolicConstant(c, "Key_Dead_Breve", "int",
                                 Value(int(0x01001255))),
            new SymbolicConstant(c, "Key_Dead_Abovedot", "int",
                                 Value(int(0x01001256))),
            new SymbolicConstant(c, "Key_Dead_Diaeresis", "int",
                                 Value(int(0x01001257))),
            new SymbolicConstant(c, "Key_Dead_Abovering", "int",
                                 Value(int(0x01001258))),
            new SymbolicConstant(c, "Key_Dead_Doubleacute", "int",
                                 Value(int(0x01001259))),
            new SymbolicConstant(c, "Key_Dead_Caron", "int",
                                 Value(int(0x0100125a))),
            new SymbolicConstant(c, "Key_Dead_Cedilla", "int",
                                 Value(int(0x0100125b))),
            new SymbolicConstant(c, "Key_Dead_Ogonek", "int",
                                 Value(int(0x0100125c))),
            new SymbolicConstant(c, "Key_Dead_Iota", "int",
                                 Value(int(0x0100125d))),
            new SymbolicConstant(c, "Key_Dead_Voiced_Sound", "int",
                                 Value(int(0x0100125e))),
            new SymbolicConstant(c, "Key_Dead_Semivoiced_Sound", "int",
                                 Value(int(0x0100125f))),
            new SymbolicConstant(c, "Key_Dead_Belowdot", "int",
                                 Value(int(0x01001260))),
            new SymbolicConstant(c, "Key_Dead_Hook", "int",
                                 Value(int(0x01001261))),
            new SymbolicConstant(c, "Key_Dead_Horn", "int",
                                 Value(int(0x01001262))),
            new SymbolicConstant(c, "Key_Back", "int", Value(int(0x01000061))),
            new SymbolicConstant(c, "Key_Forward", "int",
                                 Value(int(0x01000062))),
            new SymbolicConstant(c, "Key_Stop", "int", Value(int(0x01000063))),
            new SymbolicConstant(c, "Key_Refresh", "int",
                                 Value(int(0x01000064))),
            new SymbolicConstant(c, "Key_VolumeDown", "int",
                                 Value(int(0x01000070))),
            new SymbolicConstant(c, "Key_VolumeMute", "int",
                                 Value(int(0x01000071))),
            new SymbolicConstant(c, "Key_VolumeUp", "int",
                                 Value(int(0x01000072))),
            new SymbolicConstant(c, "Key_BassBoost", "int",
                                 Value(int(0x01000073))),
            new SymbolicConstant(c, "Key_BassUp", "int",
                                 Value(int(0x01000074))),
            new SymbolicConstant(c, "Key_BassDown", "int",
                                 Value(int(0x01000075))),
            new SymbolicConstant(c, "Key_TrebleUp", "int",
                                 Value(int(0x01000076))),
            new SymbolicConstant(c, "Key_TrebleDown", "int",
                                 Value(int(0x01000077))),
            new SymbolicConstant(c, "Key_MediaPlay", "int",
                                 Value(int(0x01000080))),
            new SymbolicConstant(c, "Key_MediaStop", "int",
                                 Value(int(0x01000081))),
            new SymbolicConstant(c, "Key_MediaPrevious", "int",
                                 Value(int(0x01000082))),
            new SymbolicConstant(c, "Key_MediaNext", "int",
                                 Value(int(0x01000083))),
            new SymbolicConstant(c, "Key_MediaRecord", "int",
                                 Value(int(0x01000084))),
            new SymbolicConstant(c, "Key_HomePage", "int",
                                 Value(int(0x01000090))),
            new SymbolicConstant(c, "Key_Favorites", "int",
                                 Value(int(0x01000091))),
            new SymbolicConstant(c, "Key_Search", "int",
                                 Value(int(0x01000092))),
            new SymbolicConstant(c, "Key_Standby", "int",
                                 Value(int(0x01000093))),
            new SymbolicConstant(c, "Key_OpenUrl", "int",
                                 Value(int(0x01000094))),
            new SymbolicConstant(c, "Key_LaunchMail", "int",
                                 Value(int(0x010000a0))),
            new SymbolicConstant(c, "Key_LaunchMedia", "int",
                                 Value(int(0x010000a1))),
            new SymbolicConstant(c, "Key_Launch0", "int",
                                 Value(int(0x010000a2))),
            new SymbolicConstant(c, "Key_Launch1", "int",
                                 Value(int(0x010000a3))),
            new SymbolicConstant(c, "Key_Launch2", "int",
                                 Value(int(0x010000a4))),
            new SymbolicConstant(c, "Key_Launch3", "int",
                                 Value(int(0x010000a5))),
            new SymbolicConstant(c, "Key_Launch4", "int",
                                 Value(int(0x010000a6))),
            new SymbolicConstant(c, "Key_Launch5", "int",
                                 Value(int(0x010000a7))),
            new SymbolicConstant(c, "Key_Launch6", "int",
                                 Value(int(0x010000a8))),
            new SymbolicConstant(c, "Key_Launch7", "int",
                                 Value(int(0x010000a9))),
            new SymbolicConstant(c, "Key_Launch8", "int",
                                 Value(int(0x010000aa))),
            new SymbolicConstant(c, "Key_Launch9", "int",
                                 Value(int(0x010000ab))),
            new SymbolicConstant(c, "Key_LaunchA", "int",
                                 Value(int(0x010000ac))),
            new SymbolicConstant(c, "Key_LaunchB", "int",
                                 Value(int(0x010000ad))),
            new SymbolicConstant(c, "Key_LaunchC", "int",
                                 Value(int(0x010000ae))),
            new SymbolicConstant(c, "Key_LaunchD", "int",
                                 Value(int(0x010000af))),
            new SymbolicConstant(c, "Key_LaunchE", "int",
                                 Value(int(0x010000b0))),
            new SymbolicConstant(c, "Key_LaunchF", "int",
                                 Value(int(0x010000b1))),
            new SymbolicConstant(c, "Key_MediaLast", "int",
                                 Value(int(0x0100ffff))),
            new SymbolicConstant(c, "Key_unknown", "int",
                                 Value(int(0x01ffffff))),
            new SymbolicConstant(c, "Key_Call", "int", Value(int(0x01100004))),
            new SymbolicConstant(c, "Key_Context1", "int",
                                 Value(int(0x01100000))),
            new SymbolicConstant(c, "Key_Context2", "int",
                                 Value(int(0x01100001))),
            new SymbolicConstant(c, "Key_Context3", "int",
                                 Value(int(0x01100002))),
            new SymbolicConstant(c, "Key_Context4", "int",
                                 Value(int(0x01100003))),
            new SymbolicConstant(c, "Key_Flip", "int", Value(int(0x01100006))),
            new SymbolicConstant(c, "Key_Hangup", "int",
                                 Value(int(0x01100005))),
            new SymbolicConstant(c, "Key_No", "int", Value(int(0x01010002))),
            new SymbolicConstant(c, "Key_Select", "int",
                                 Value(int(0x01010000))),
            new SymbolicConstant(c, "Key_Yes", "int", Value(int(0x01010001))),
            new SymbolicConstant(c, "Key_Execute", "int",
                                 Value(int(0x01020003))),
            new SymbolicConstant(c, "Key_Printer", "int",
                                 Value(int(0x01020002))),
            new SymbolicConstant(c, "Key_Play", "int", Value(int(0x01020005))),
            new SymbolicConstant(c, "Key_Sleep", "int", Value(int(0x01020004))),
            new SymbolicConstant(c, "Key_Zoom", "int", Value(int(0x01020006))),
            new SymbolicConstant(c, "Key_Cancel", "int",
                                 Value(int(0x01020001))),
            new Alias(c, "KeyboardModifier", "int"),
            new SymbolicConstant(c, "NoModifier", "int",
                                 Value(int(0x00000000))),
            new SymbolicConstant(c, "ShiftModifier", "int",
                                 Value(int(0x02000000))),
            new SymbolicConstant(c, "ControlModifier", "int",
                                 Value(int(0x04000000))),
            new SymbolicConstant(c, "AltModifier", "int",
                                 Value(int(0x08000000))),
            new SymbolicConstant(c, "MetaModifier", "int",
                                 Value(int(0x10000000))),
            new SymbolicConstant(c, "KeypadModifier", "int",
                                 Value(int(0x20000000))),
            new SymbolicConstant(c, "GroupSwitchModifier", "int",
                                 Value(int(0x40000000))),
            new Alias(c, "LayoutDirection", "int"),
            new SymbolicConstant(c, "LeftToRight", "int", Value(int(0))),
            new SymbolicConstant(c, "RightToLeft", "int", Value(int(1))),
            new Alias(c, "MaskMode", "int"),
            new SymbolicConstant(c, "MaskInColor", "int", Value(int(0))),
            new SymbolicConstant(c, "MaskOutColor", "int", Value(int(1))),
            new Alias(c, "MatchFlag", "int"),
            new SymbolicConstant(c, "MatchExactly", "int", Value(int(0))),
            new SymbolicConstant(c, "MatchFixedString", "int", Value(int(8))),
            new SymbolicConstant(c, "MatchContains", "int", Value(int(1))),
            new SymbolicConstant(c, "MatchStartsWith", "int", Value(int(2))),
            new SymbolicConstant(c, "MatchEndsWith", "int", Value(int(3))),
            new SymbolicConstant(c, "MatchCaseSensitive", "int",
                                 Value(int(16))),
            new SymbolicConstant(c, "MatchRegExp", "int", Value(int(4))),
            new SymbolicConstant(c, "MatchWildcard", "int", Value(int(5))),
            new SymbolicConstant(c, "MatchWrap", "int", Value(int(32))),
            new SymbolicConstant(c, "MatchRecursive", "int", Value(int(64))),
            new Alias(c, "Modifier", "int"),
            new SymbolicConstant(c, "SHIFT", "int",
                                 Value(int(Qt::ShiftModifier))),
            new SymbolicConstant(c, "META", "int",
                                 Value(int(Qt::MetaModifier))),
            new SymbolicConstant(c, "CTRL", "int",
                                 Value(int(Qt::ControlModifier))),
            new SymbolicConstant(c, "ALT", "int", Value(int(Qt::AltModifier))),
            new SymbolicConstant(c, "UNICODE_ACCEL", "int",
                                 Value(int(0x00000000))),
            new Alias(c, "MouseButton", "int"),
            new SymbolicConstant(c, "NoButton", "int", Value(int(0x00000000))),
            new SymbolicConstant(c, "LeftButton", "int",
                                 Value(int(0x00000001))),
            new SymbolicConstant(c, "RightButton", "int",
                                 Value(int(0x00000002))),
            new SymbolicConstant(c, "MidButton", "int", Value(int(0x00000004))),
            new SymbolicConstant(c, "XButton1", "int", Value(int(0x00000008))),
            new SymbolicConstant(c, "XButton2", "int", Value(int(0x00000010))),
            new Alias(c, "Orientation", "int"),
            new SymbolicConstant(c, "Horizontal", "int", Value(int(0x1))),
            new SymbolicConstant(c, "Vertical", "int", Value(int(0x2))),
            new Alias(c, "PenCapStyle", "int"),
            new SymbolicConstant(c, "SquareCap", "int",
                                 Value(int(Qt::FlatCap))),
            new SymbolicConstant(c, "FlatCap", "int", Value(int(0x00))),
            new SymbolicConstant(c, "SquareCap", "int", Value(int(0x10))),
            new SymbolicConstant(c, "RoundCap", "int", Value(int(0x20))),
            new Alias(c, "PenJoinStyle", "int"),
            new SymbolicConstant(c, "BevelJoin", "int",
                                 Value(int(Qt::MiterJoin))),
            new SymbolicConstant(c, "MiterJoin", "int", Value(int(0x00))),
            new SymbolicConstant(c, "BevelJoin", "int", Value(int(0x40))),
            new SymbolicConstant(c, "RoundJoin", "int", Value(int(0x80))),
            new SymbolicConstant(c, "SvgMiterJoin", "int", Value(int(0x100))),
            new Alias(c, "PenStyle", "int"),
            new SymbolicConstant(c, "SolidLine", "int",
                                 Value(int(Qt::DashLine))),
            new SymbolicConstant(c, "DashDotLine", "int",
                                 Value(int(Qt::DashDotDotLine))),
            new SymbolicConstant(c, "NoPen", "int", Value(int(0))),
            new SymbolicConstant(c, "SolidLine", "int", Value(int(1))),
            new SymbolicConstant(c, "DashLine", "int", Value(int(2))),
            new SymbolicConstant(c, "DotLine", "int", Value(int(3))),
            new SymbolicConstant(c, "DashDotLine", "int", Value(int(4))),
            new SymbolicConstant(c, "DashDotDotLine", "int", Value(int(5))),
            new SymbolicConstant(c, "CustomDashLine", "int", Value(int(6))),
            new Alias(c, "ScrollBarPolicy", "int"),
            new SymbolicConstant(c, "ScrollBarAsNeeded", "int", Value(int(0))),
            new SymbolicConstant(c, "ScrollBarAlwaysOff", "int", Value(int(1))),
            new SymbolicConstant(c, "ScrollBarAlwaysOn", "int", Value(int(2))),
            new Alias(c, "ShortcutContext", "int"),
            new SymbolicConstant(c, "WidgetShortcut", "int", Value(int(0))),
            new SymbolicConstant(c, "WidgetWithChildrenShortcut", "int",
                                 Value(int(3))),
            new SymbolicConstant(c, "WindowShortcut", "int", Value(int(1))),
            new SymbolicConstant(c, "ApplicationShortcut", "int",
                                 Value(int(2))),
            new Alias(c, "SizeHint", "int"),
            new SymbolicConstant(c, "MinimumSize", "int", Value(int(0))),
            new SymbolicConstant(c, "PreferredSize", "int", Value(int(1))),
            new SymbolicConstant(c, "MaximumSize", "int", Value(int(2))),
            new SymbolicConstant(c, "MinimumDescent", "int", Value(int(3))),
            new Alias(c, "SizeMode", "int"),
            new SymbolicConstant(c, "AbsoluteSize", "int", Value(int(0))),
            new SymbolicConstant(c, "RelativeSize", "int", Value(int(1))),
            new Alias(c, "SortOrder", "int"),
            new SymbolicConstant(c, "AscendingOrder", "int", Value(int(0))),
            new SymbolicConstant(c, "DescendingOrder", "int", Value(int(1))),
            new Alias(c, "TextElideMode", "int"),
            new SymbolicConstant(c, "ElideLeft", "int", Value(int(0))),
            new SymbolicConstant(c, "ElideRight", "int", Value(int(1))),
            new SymbolicConstant(c, "ElideMiddle", "int", Value(int(2))),
            new SymbolicConstant(c, "ElideNone", "int", Value(int(3))),
            new Alias(c, "TextFlag", "int"),
            new SymbolicConstant(c, "TextSingleLine", "int",
                                 Value(int(0x0100))),
            new SymbolicConstant(c, "TextDontClip", "int", Value(int(0x0200))),
            new SymbolicConstant(c, "TextExpandTabs", "int",
                                 Value(int(0x0400))),
            new SymbolicConstant(c, "TextShowMnemonic", "int",
                                 Value(int(0x0800))),
            new SymbolicConstant(c, "TextWordWrap", "int", Value(int(0x1000))),
            new SymbolicConstant(c, "TextWrapAnywhere", "int",
                                 Value(int(0x2000))),
            new SymbolicConstant(c, "TextHideMnemonic", "int",
                                 Value(int(0x8000))),
            new SymbolicConstant(c, "TextDontPrint", "int", Value(int(0x4000))),
            new SymbolicConstant(c, "IncludeTrailingSpaces", "int",
                                 Value(int(Qt::TextIncludeTrailingSpaces))),
            new SymbolicConstant(c, "TextIncludeTrailingSpaces", "int",
                                 Value(int(0x08000000))),
            new SymbolicConstant(c, "TextJustificationForced", "int",
                                 Value(int(0x10000))),
            new Alias(c, "TextFormat", "int"),
            new SymbolicConstant(c, "PlainText", "int", Value(int(0))),
            new SymbolicConstant(c, "RichText", "int", Value(int(1))),
            new SymbolicConstant(c, "AutoText", "int", Value(int(2))),
            new SymbolicConstant(c, "LogText", "int", Value(int(3))),
            new Alias(c, "TextInteractionFlag", "int"),
            new SymbolicConstant(c, "NoTextInteraction", "int", Value(int(0))),
            new SymbolicConstant(c, "TextSelectableByMouse", "int",
                                 Value(int(1))),
            new SymbolicConstant(c, "TextSelectableByKeyboard", "int",
                                 Value(int(2))),
            new SymbolicConstant(c, "LinksAccessibleByMouse", "int",
                                 Value(int(4))),
            new SymbolicConstant(c, "LinksAccessibleByKeyboard", "int",
                                 Value(int(8))),
            new SymbolicConstant(c, "TextEditable", "int", Value(int(16))),
            new SymbolicConstant(
                c, "TextEditorInteraction", "int",
                Value(int(Qt::TextSelectableByMouse
                          | Qt::TextSelectableByKeyboard | Qt::TextEditable))),
            new SymbolicConstant(
                c, "TextBrowserInteraction", "int",
                Value(int(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse
                          | Qt::LinksAccessibleByKeyboard))),
            new Alias(c, "TimeSpec", "int"),
            new SymbolicConstant(c, "LocalTime", "int", Value(int(0))),
            new SymbolicConstant(c, "UTC", "int", Value(int(1))),
            new SymbolicConstant(c, "OffsetFromUTC", "int", Value(int(2))),
            new Alias(c, "ToolBarArea", "int"),
            new SymbolicConstant(c, "LeftToolBarArea", "int", Value(int(0x1))),
            new SymbolicConstant(c, "RightToolBarArea", "int", Value(int(0x2))),
            new SymbolicConstant(c, "TopToolBarArea", "int", Value(int(0x4))),
            new SymbolicConstant(c, "BottomToolBarArea", "int",
                                 Value(int(0x8))),
            new SymbolicConstant(c, "AllToolBarAreas", "int",
                                 Value(int(Qt::ToolBarArea_Mask))),
            new SymbolicConstant(c, "NoToolBarArea", "int", Value(int(0))),
            new Alias(c, "ToolButtonStyle", "int"),
            new SymbolicConstant(c, "ToolButtonIconOnly", "int", Value(int(0))),
            new SymbolicConstant(c, "ToolButtonTextOnly", "int", Value(int(1))),
            new SymbolicConstant(c, "ToolButtonTextBesideIcon", "int",
                                 Value(int(2))),
            new SymbolicConstant(c, "ToolButtonTextUnderIcon", "int",
                                 Value(int(3))),
            new Alias(c, "TransformationMode", "int"),
            new SymbolicConstant(c, "FastTransformation", "int", Value(int(0))),
            new SymbolicConstant(c, "SmoothTransformation", "int",
                                 Value(int(1))),
            new Alias(c, "UIEffect", "int"),
            new SymbolicConstant(c, "UI_AnimateMenu", "int", Value(int(1))),
            new SymbolicConstant(c, "UI_FadeMenu", "int", Value(int(2))),
            new SymbolicConstant(c, "UI_AnimateCombo", "int", Value(int(3))),
            new SymbolicConstant(c, "UI_AnimateTooltip", "int", Value(int(4))),
            new SymbolicConstant(c, "UI_FadeTooltip", "int", Value(int(5))),
            new SymbolicConstant(c, "UI_AnimateToolBox", "int", Value(int(6))),
            new Alias(c, "WhiteSpaceMode", "int"),
            new SymbolicConstant(c, "WhiteSpaceNormal", "int", Value(int(0))),
            new SymbolicConstant(c, "WhiteSpacePre", "int", Value(int(1))),
            new SymbolicConstant(c, "WhiteSpaceNoWrap", "int", Value(int(2))),
            new Alias(c, "WidgetAttribute", "int"),
            new SymbolicConstant(c, "WA_AcceptDrops", "int", Value(int(78))),
            new SymbolicConstant(c, "WA_AlwaysShowToolTips", "int",
                                 Value(int(84))),
            new SymbolicConstant(c, "WA_ContentsPropagated", "int",
                                 Value(int(3))),
            new SymbolicConstant(c, "WA_CustomWhatsThis", "int",
                                 Value(int(47))),
            new SymbolicConstant(c, "WA_DeleteOnClose", "int", Value(int(55))),
            new SymbolicConstant(c, "WA_Disabled", "int", Value(int(0))),
            new SymbolicConstant(c, "WA_ForceDisabled", "int", Value(int(32))),
            new SymbolicConstant(c, "WA_ForceUpdatesDisabled", "int",
                                 Value(int(59))),
            new SymbolicConstant(c, "WA_GroupLeader", "int", Value(int(72))),
            new SymbolicConstant(c, "WA_Hover", "int", Value(int(74))),
            new SymbolicConstant(c, "WA_InputMethodEnabled", "int",
                                 Value(int(14))),
            new SymbolicConstant(c, "WA_KeyboardFocusChange", "int",
                                 Value(int(77))),
            new SymbolicConstant(c, "WA_KeyCompression", "int", Value(int(33))),
            new SymbolicConstant(c, "WA_LayoutOnEntireRect", "int",
                                 Value(int(48))),
            new SymbolicConstant(c, "WA_LayoutUsesWidgetRect", "int",
                                 Value(int(92))),
            new SymbolicConstant(c, "WA_MacNoClickThrough", "int",
                                 Value(int(12))),
            new SymbolicConstant(c, "WA_MacOpaqueSizeGrip", "int",
                                 Value(int(85))),
            new SymbolicConstant(c, "WA_MacShowFocusRect", "int",
                                 Value(int(88))),
            new SymbolicConstant(c, "WA_MacNormalSize", "int", Value(int(89))),
            new SymbolicConstant(c, "WA_MacSmallSize", "int", Value(int(90))),
            new SymbolicConstant(c, "WA_MacMiniSize", "int", Value(int(91))),
            new SymbolicConstant(c, "WA_MacVariableSize", "int",
                                 Value(int(102))),
            new SymbolicConstant(c, "WA_MacBrushedMetal", "int",
                                 Value(int(46))),
            new SymbolicConstant(c, "WA_Mapped", "int", Value(int(11))),
            new SymbolicConstant(c, "WA_MouseNoMask", "int", Value(int(71))),
            new SymbolicConstant(c, "WA_MouseTracking", "int", Value(int(2))),
            new SymbolicConstant(c, "WA_Moved", "int", Value(int(43))),
            new SymbolicConstant(c, "WA_MSWindowsUseDirect3D", "int",
                                 Value(int(94))),
            new SymbolicConstant(c, "WA_NoBackground", "int",
                                 Value(int(Qt::WA_OpaquePaintEvent))),
            new SymbolicConstant(c, "WA_NoChildEventsForParent", "int",
                                 Value(int(58))),
            new SymbolicConstant(c, "WA_NoChildEventsFromChildren", "int",
                                 Value(int(39))),
            new SymbolicConstant(c, "WA_NoMouseReplay", "int", Value(int(54))),
            new SymbolicConstant(c, "WA_NoMousePropagation", "int",
                                 Value(int(73))),
            new SymbolicConstant(c, "WA_TransparentForMouseEvents", "int",
                                 Value(int(51))),
            new SymbolicConstant(c, "WA_NoSystemBackground", "int",
                                 Value(int(9))),
            new SymbolicConstant(c, "WA_OpaquePaintEvent", "int",
                                 Value(int(4))),
            new SymbolicConstant(c, "WA_OutsideWSRange", "int", Value(int(49))),
            new SymbolicConstant(c, "WA_PaintOnScreen", "int", Value(int(8))),
            new SymbolicConstant(c, "WA_PaintOutsidePaintEvent", "int",
                                 Value(int(13))),
            new SymbolicConstant(c, "WA_PaintUnclipped", "int", Value(int(52))),
            new SymbolicConstant(c, "WA_PendingMoveEvent", "int",
                                 Value(int(34))),
            new SymbolicConstant(c, "WA_PendingResizeEvent", "int",
                                 Value(int(35))),
            new SymbolicConstant(c, "WA_QuitOnClose", "int", Value(int(76))),
            new SymbolicConstant(c, "WA_Resized", "int", Value(int(42))),
            new SymbolicConstant(c, "WA_RightToLeft", "int", Value(int(56))),
            new SymbolicConstant(c, "WA_SetCursor", "int", Value(int(38))),
            new SymbolicConstant(c, "WA_SetFont", "int", Value(int(37))),
            new SymbolicConstant(c, "WA_SetPalette", "int", Value(int(36))),
            new SymbolicConstant(c, "WA_SetStyle", "int", Value(int(86))),
            new SymbolicConstant(c, "WA_ShowModal", "int", Value(int(70))),
            new SymbolicConstant(c, "WA_StaticContents", "int", Value(int(5))),
            new SymbolicConstant(c, "WA_StyleSheet", "int", Value(int(97))),
            new SymbolicConstant(c, "WA_TranslucentBackground", "int",
                                 Value(int(120))),
            new SymbolicConstant(c, "WA_UnderMouse", "int", Value(int(1))),
            new SymbolicConstant(c, "WA_UpdatesDisabled", "int",
                                 Value(int(10))),
            new SymbolicConstant(c, "WA_WindowModified", "int", Value(int(41))),
            new SymbolicConstant(c, "WA_WindowPropagation", "int",
                                 Value(int(80))),
            new SymbolicConstant(c, "WA_MacAlwaysShowToolWindow", "int",
                                 Value(int(96))),
            new SymbolicConstant(c, "WA_SetLocale", "int", Value(int(87))),
            new SymbolicConstant(c, "WA_StyledBackground", "int",
                                 Value(int(93))),
            new SymbolicConstant(c, "WA_ShowWithoutActivating", "int",
                                 Value(int(98))),
            new SymbolicConstant(c, "WA_NativeWindow", "int", Value(int(100))),
            new SymbolicConstant(c, "WA_DontCreateNativeAncestors", "int",
                                 Value(int(101))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeDesktop", "int",
                                 Value(int(104))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeDock", "int",
                                 Value(int(105))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeToolBar", "int",
                                 Value(int(106))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeMenu", "int",
                                 Value(int(107))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeUtility", "int",
                                 Value(int(108))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeSplash", "int",
                                 Value(int(109))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeDialog", "int",
                                 Value(int(110))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeDropDownMenu", "int",
                                 Value(int(111))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypePopupMenu", "int",
                                 Value(int(112))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeToolTip", "int",
                                 Value(int(113))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeNotification", "int",
                                 Value(int(114))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeCombo", "int",
                                 Value(int(115))),
            new SymbolicConstant(c, "WA_X11NetWmWindowTypeDND", "int",
                                 Value(int(116))),
            new SymbolicConstant(c, "WA_MacFrameworkScaled", "int",
                                 Value(int(117))),
            new Alias(c, "WindowFrameSection", "int"),
            new SymbolicConstant(c, "NoSection", "int", Value(int(0))),
            new SymbolicConstant(c, "LeftSection", "int", Value(int(1))),
            new SymbolicConstant(c, "TopLeftSection", "int", Value(int(2))),
            new SymbolicConstant(c, "TopSection", "int", Value(int(3))),
            new SymbolicConstant(c, "TopRightSection", "int", Value(int(4))),
            new SymbolicConstant(c, "RightSection", "int", Value(int(5))),
            new SymbolicConstant(c, "BottomRightSection", "int", Value(int(6))),
            new SymbolicConstant(c, "BottomSection", "int", Value(int(7))),
            new SymbolicConstant(c, "BottomLeftSection", "int", Value(int(8))),
            new SymbolicConstant(c, "TitleBarArea", "int", Value(int(9))),
            new Alias(c, "WindowModality", "int"),
            new SymbolicConstant(c, "NonModal", "int", Value(int(0))),
            new SymbolicConstant(c, "WindowModal", "int", Value(int(1))),
            new SymbolicConstant(c, "ApplicationModal", "int", Value(int(2))),
            new Alias(c, "WindowState", "int"),
            new SymbolicConstant(c, "WindowNoState", "int",
                                 Value(int(0x00000000))),
            new SymbolicConstant(c, "WindowMinimized", "int",
                                 Value(int(0x00000001))),
            new SymbolicConstant(c, "WindowMaximized", "int",
                                 Value(int(0x00000002))),
            new SymbolicConstant(c, "WindowFullScreen", "int",
                                 Value(int(0x00000004))),
            new SymbolicConstant(c, "WindowActive", "int",
                                 Value(int(0x00000008))),
            new Alias(c, "WindowType", "int"),
            new SymbolicConstant(c, "Widget", "int", Value(int(0x00000000))),
            new SymbolicConstant(c, "Window", "int", Value(int(0x00000001))),
            new SymbolicConstant(c, "Dialog", "int",
                                 Value(int(0x00000002 | Qt::Window))),
            new SymbolicConstant(c, "Sheet", "int",
                                 Value(int(0x00000004 | Qt::Window))),
            new SymbolicConstant(c, "Drawer", "int",
                                 Value(int(0x00000006 | Qt::Window))),
            new SymbolicConstant(c, "Popup", "int",
                                 Value(int(0x00000008 | Qt::Window))),
            new SymbolicConstant(c, "Tool", "int",
                                 Value(int(0x0000000a | Qt::Window))),
            new SymbolicConstant(c, "ToolTip", "int",
                                 Value(int(0x0000000c | Qt::Window))),
            new SymbolicConstant(c, "SplashScreen", "int",
                                 Value(int(0x0000000e | Qt::Window))),
            new SymbolicConstant(c, "Desktop", "int",
                                 Value(int(0x00000010 | Qt::Window))),
            new SymbolicConstant(c, "SubWindow", "int", Value(int(0x00000012))),
            new SymbolicConstant(c, "MSWindowsFixedSizeDialogHint", "int",
                                 Value(int(0x00000100))),
            new SymbolicConstant(c, "MSWindowsOwnDC", "int",
                                 Value(int(0x00000200))),
            new SymbolicConstant(c, "X11BypassWindowManagerHint", "int",
                                 Value(int(0x00000400))),
            new SymbolicConstant(c, "FramelessWindowHint", "int",
                                 Value(int(0x00000800))),
            new SymbolicConstant(c, "CustomizeWindowHint", "int",
                                 Value(int(0x02000000))),
            new SymbolicConstant(c, "WindowTitleHint", "int",
                                 Value(int(0x00001000))),
            new SymbolicConstant(c, "WindowSystemMenuHint", "int",
                                 Value(int(0x00002000))),
            new SymbolicConstant(c, "WindowMinimizeButtonHint", "int",
                                 Value(int(0x00004000))),
            new SymbolicConstant(c, "WindowMaximizeButtonHint", "int",
                                 Value(int(0x00008000))),
            new SymbolicConstant(c, "WindowMinMaxButtonsHint", "int",
                                 Value(int(Qt::WindowMinimizeButtonHint
                                           | Qt::WindowMaximizeButtonHint))),
            new SymbolicConstant(c, "WindowCloseButtonHint", "int",
                                 Value(int(0x08000000))),
            new SymbolicConstant(c, "WindowContextHelpButtonHint", "int",
                                 Value(int(0x00010000))),
            new SymbolicConstant(c, "MacWindowToolBarButtonHint", "int",
                                 Value(int(0x10000000))),
            new SymbolicConstant(c, "BypassGraphicsProxyWidget", "int",
                                 Value(int(0x20000000))),
            new SymbolicConstant(c, "WindowShadeButtonHint", "int",
                                 Value(int(0x00020000))),
            new SymbolicConstant(c, "WindowStaysOnTopHint", "int",
                                 Value(int(0x00040000))),
            new SymbolicConstant(c, "WindowStaysOnBottomHint", "int",
                                 Value(int(0x04000000))),
            new SymbolicConstant(c, "WindowOkButtonHint", "int",
                                 Value(int(0x00080000))),
            new SymbolicConstant(c, "WindowCancelButtonHint", "int",
                                 Value(int(0x00100000))),
            new SymbolicConstant(c, "WindowType_Mask", "int",
                                 Value(int(0x000000ff))),

            EndArguments);

        addSymbols(
            qt,

            //
            //  Custom Widgets
            //

            // new HintWidgetType(c, "HintWidget", widgetType),

            //
            //  Extra Qt Widgets
            //

            new QtColorTriangleType(c, "QtColorTriangle", widgetType),

            //
            //  Top level functions
            //

            new Function(c, "connect", Mu::connect, None, Return, "int",
                         Parameters, new Param(c, "sender", "qt.QObject"),
                         new Param(c, "signal", "?function"),
                         new Param(c, "callback", "?function"), End),

            new Function(c, "connect", Mu::connect2, None, Return, "int",
                         Parameters, new Param(c, "sender", "qt.QObject"),
                         new Param(c, "signal", "string"),
                         new Param(c, "callback", "?function"), End),

            new Function(c, "exists", Mu::qobjectExists, None, Return, "bool",
                         Parameters, new Param(c, "qobj", "qt.QObject"), End),

            new Function(c, "invoke", Mu::qinvoke, None, Return, "bool", Args,
                         "qt.QObject", "int", "string", Optional, "?varargs",
                         Maximum, 6, End),

            EndArguments);

        addSymbol(new Function(c, "loadUIFile", Mu::loadUIFile, None, Return,
                               "qt.QObject", Parameters,
                               new Param(c, "filename", "string"),
                               new Param(c, "parent", "qt.QWidget"), End));

        globalScope()->addSymbols(
            new Function(c, "==", Mu::qobjectEquality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QObject"),
                         new Param(c, "b", "qt.QObject"), End),

            new Function(c, "!=", Mu::qobjectInequality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QObject"),
                         new Param(c, "b", "qt.QObject"), End),

            new Function(c, "==", Mu::qobjectEquality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QStandardItem"),
                         new Param(c, "b", "qt.QStandardItem"), End),

            new Function(c, "!=", Mu::qobjectInequality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QStandardItem"),
                         new Param(c, "b", "qt.QStandardItem"), End),

            new Function(c, "==", Mu::qobjectEquality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QListWidgetItem"),
                         new Param(c, "b", "qt.QListWidgetItem"), End),

            new Function(c, "!=", Mu::qobjectInequality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QListWidgetItem"),
                         new Param(c, "b", "qt.QListWidgetItem"), End),

            new Function(c, "==", Mu::qobjectEquality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QTreeWidgetItem"),
                         new Param(c, "b", "qt.QTreeWidgetItem"), End),

            new Function(c, "!=", Mu::qobjectInequality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QTreeWidgetItem"),
                         new Param(c, "b", "qt.QTreeWidgetItem"), End),

            new Function(c, "==", Mu::qobjectEquality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QTableWidgetItem"),
                         new Param(c, "b", "qt.QTableWidgetItem"), End),

            new Function(c, "!=", Mu::qobjectInequality, Op, Return, "bool",
                         Parameters, new Param(c, "a", "qt.QTableWidgetItem"),
                         new Param(c, "b", "qt.QTableWidgetItem"), End),

            EndArguments);

        loadGlobals();

        // cout << "QT LOAD END" << endl;
    }

} // namespace Mu
