//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

//
// Defining WIN32_LEAN_AND_MEAN
// prevents msvc redefinition errors/warnings between
// windows.h and winsock headers.
//
#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <type_traits>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <Mu/Function.h>
#include <Mu/NodePrinter.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
// #include <MuQt6/qtModuleIncludes.h>
#include <MuQt6/qtUtils.h>
#include <MuQt6/QObjectType.h>
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QStandardItemType.h>
#include <MuQt6/QListWidgetItemType.h>
#include <MuQt6/QTableWidgetItemType.h>
#include <MuQt6/QTreeWidgetItemType.h>
#include <MuQt6/QUrlType.h>
#include <MuQt6/QVariantType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QFontType.h>
#include <MuQt6/QColorType.h>
#include <MuQt6/QPointType.h>
#include <MuQt6/QRectType.h>
#include <MuQt6/QItemSelectionType.h>
//
//  The QSignalSpy-based implementation reads QSignalSpy's private members,
//  hence the "private" hack. RV_MUQT_SIGNALSPY_OWN_CONNECT comes from the
//  command line (see MuQt6/CMakeLists.txt), so it is already visible here.
//
#ifndef RV_MUQT_SIGNALSPY_OWN_CONNECT
#define private public
#endif
#include <MuQt6/SignalSpy.h>
#ifndef RV_MUQT_SIGNALSPY_OWN_CONNECT
#undef private
#endif

namespace Mu
{
    using namespace std;

#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT

    const QList<int>& SignalSpy::signalArgTypes() const { return _signalArgTypes; }

    //
    //  Resolve the signal and connect it to this object. This is what
    //  QSignalSpy's constructor used to do for us before Qt 6.8 made it a
    //  non-QObject; the incoming string is in SIGNAL() form, i.e. the
    //  QSIGNAL_CODE digit followed by the normalized signature (see
    //  signalName() in qtModule.cpp).
    //
    bool SignalSpy::connectToSignal(QObject* sender, const char* sig)
    {
        if (!sender || !sig)
        {
            cout << "WARNING: SignalSpy: null sender or signal" << endl;
            return false;
        }

        //  Skip the SIGNAL() code digit if present.
        const char* signature = sig;
        if (((signature[0] - '0') & 0x03) == QSIGNAL_CODE)
            signature++;

        const QByteArray name = QMetaObject::normalizedSignature(signature);
        const QMetaObject* mo = sender->metaObject();
        const int index = mo->indexOfMethod(name.constData());

        if (index < 0)
        {
            cout << "WARNING: SignalSpy: no such signal: " << name.constData() << endl;
            return false;
        }

        const QMetaMethod member = mo->method(index);

        //
        //  Remember the metatypes of the signal's arguments. qt_metacall()
        //  needs them to turn the void** it receives into QVariants. Enum and
        //  flag arguments are often not registered metatypes, so fall back to
        //  asking the sender to register them, exactly as QSignalSpy did.
        //
        _signalArgTypes.reserve(member.parameterCount());

        for (int i = 0; i < member.parameterCount(); i++)
        {
            QMetaType type = member.parameterMetaType(i);

            if (!type.isValid())
            {
                void* argv[] = {&type, &i};
                QMetaObject::metacall(sender, QMetaObject::RegisterMethodArgumentMetaType, member.methodIndex(), argv);
            }

            if (!type.isValid())
            {
                cout << "WARNING: SignalSpy: unhandled argument type " << member.parameterTypeName(i).constData() << " of "
                     << name.constData() << ": use qRegisterMetaType to register it" << endl;
            }

            _signalArgTypes << type.id();
        }

        if (!QMetaObject::connect(sender, index, this, QObject::staticMetaObject.methodCount(), Qt::DirectConnection, nullptr))
        {
            cout << "WARNING: SignalSpy: failed to connect to " << name.constData() << endl;
            return false;
        }

        return true;
    }

#else // !RV_MUQT_SIGNALSPY_OWN_CONNECT

    static_assert(std::is_base_of<QObject, QSignalSpy>::value,
                  "QSignalSpy is not a QObject with this Qt version, so SignalSpy cannot derive from it. "
                  "Lower the QT_VERSION bound guarding RV_MUQT_SIGNALSPY_OWN_CONNECT in SignalSpy.h.");

    //  QSignalSpy::args is private; reachable here thanks to the "private" hack above.
    const QList<int>& SignalSpy::signalArgTypes() const { return args; }

#endif // RV_MUQT_SIGNALSPY_OWN_CONNECT

    SignalSpy::SignalSpy(QObject* o, const char* sig, const Function* F, Process* p)
#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
        : QObject(nullptr)
        , _F(F)
        , _process(p)
        , _env(p->callEnv())
        , _valid(false)
#else
        : QSignalSpy(o, sig)
        , _F(F)
        , _process(p)
        , _env(p->callEnv())
#endif
    {
#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
        _valid = connectToSignal(o, sig);
#endif

        const MuLangContext* c = static_cast<const MuLangContext*>(p->context());
        _argTypes.resize(F->numArgs());

        const Class* otype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QObject"), false);

        const Class* ptype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QPoint"), false);

        const Class* twtype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QTreeWidgetItem"), false);

        const Class* tbtype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QTableWidgetItem"), false);

        const Class* lwtype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QListWidgetItem"), false);

        const Class* sitype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QStandardItem"), false);

        const Class* mitype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QModelIndex"), false);

        const Class* istype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QItemSelection"), false);

        const Class* ctype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QColor"), false);

        const Class* utype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QUrl"), false);

        const Class* vtype = c->findSymbolOfTypeByQualifiedName<Class>(c->internName("qt.QVariant"), false);

        for (size_t i = 0; i < _argTypes.size(); i++)
        {
            if (F->argType(i) == c->intType())
                _argTypes[i] = IntArg;
            else if (F->argType(i) == c->stringType())
                _argTypes[i] = StringArg;
            else if (F->argType(i) == c->boolType())
                _argTypes[i] = BoolArg;
            else if (F->argType(i) == ctype)
                _argTypes[i] = ColorArg;
            else if (const Class* c = dynamic_cast<const Class*>(F->argType(i)))
            {
                if (c == ptype)
                    _argTypes[i] = PointArg;
                else if (c == twtype)
                    _argTypes[i] = TreeItemArg;
                else if (c == tbtype)
                    _argTypes[i] = TableItemArg;
                else if (c == lwtype)
                    _argTypes[i] = ListItemArg;
                else if (c == sitype)
                    _argTypes[i] = StandardItemArg;
                else if (c == mitype)
                    _argTypes[i] = ModelIndexArg;
                else if (c == istype)
                    _argTypes[i] = ItemSelectionArg;
                else if (c == utype)
                    _argTypes[i] = UrlArg;
                else if (c == vtype)
                    _argTypes[i] = VariantArg;
                else if (c->isA(otype))
                    _argTypes[i] = ObjectArg;
                else
                {
                    _argTypes[i] = UnknownArg;
                }
            }
            else
            {
                _argTypes[i] = UnknownArg;
            }

            if (_argTypes[i] == UnknownArg)
            {
                cout << "WARNING: " << sig << " not translated correctly" << endl;
            }
        }
    }

    SignalSpy::~SignalSpy() { _F = 0; }

    int SignalSpy::qt_metacall(QMetaObject::Call call, int methodId, void** a)
    {
        if (call == QMetaObject::InvokeMetaMethod)
        {
            Function::ArgumentVector args(_argTypes.size());
            const MuLangContext* c = static_cast<const MuLangContext*>(_F->context());
            bool failed = false;

            for (size_t i = 0; i < _argTypes.size(); i++)
            {
                // QMetaType type = QMetaType(signalArgTypes().at(i));
                // cout << "type = " << QMetaType::typeName(type) << endl;

                switch (_argTypes[i])
                {
                case IntArg:
                    args[i]._int = *reinterpret_cast<int*>(a[i + 1]);
                    break;

                case BoolArg:
                    args[i]._bool = *reinterpret_cast<bool*>(a[i + 1]);
                    break;

                case StringArg:
                {
                    QString* s = reinterpret_cast<QString*>(a[i + 1]);
                    args[i]._Pointer = c->stringType()->allocate(s->toUtf8().constData());
                }
                break;

                case ObjectArg:
                {
                    QObject** o = reinterpret_cast<QObject**>(a[i + 1]);
                    args[i]._Pointer = makeinstance<QObjectType>((QObjectType*)_F->argType(i), *o);
                }
                break;

                case ColorArg:
                {
                    QColor* o = reinterpret_cast<QColor*>(a[i + 1]);
                    args[i]._Pointer = makeqtype<QColorType>((Class*)_F->argType(i), *o);
                }
                break;

                case UrlArg:
                {
                    QUrl* o = reinterpret_cast<QUrl*>(a[i + 1]);
                    args[i]._Pointer = makeqtype<QUrlType>((Class*)_F->argType(i), *o);
                }
                break;

                case VariantArg:
                {
                    QVariant* v = reinterpret_cast<QVariant*>(a[i + 1]);
                    args[i]._Pointer = makeqtype<QVariantType>((Class*)_F->argType(i), *v);
                }
                break;

                case PointArg:
                {
                    QPoint* o = reinterpret_cast<QPoint*>(a[i + 1]);
                    args[i]._Pointer = makeqtype<QPointType>((Class*)_F->argType(i), *o);
                }
                break;

                case TreeItemArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QTreeWidgetItem* o = v.value<QTreeWidgetItem*>();
                    args[i]._Pointer = !o ? NULL : makeqpointer<QTreeWidgetItemType>((QTreeWidgetItemType*)_F->argType(i), o);
                }
                break;

                case TableItemArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QTableWidgetItem* o = v.value<QTableWidgetItem*>();
                    args[i]._Pointer = !o ? NULL : makeqpointer<QTableWidgetItemType>((QTableWidgetItemType*)_F->argType(i), o);
                }
                break;

                case ListItemArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QListWidgetItem* o = v.value<QListWidgetItem*>();
                    args[i]._Pointer = !o ? NULL : makeqpointer<QListWidgetItemType>((QListWidgetItemType*)_F->argType(i), o);
                }
                break;

                case StandardItemArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QStandardItem* o = v.value<QStandardItem*>();
                    args[i]._Pointer = !o ? NULL : makeqpointer<QStandardItemType>((QStandardItemType*)_F->argType(i), o);
                }
                break;

                case ModelIndexArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QModelIndex o = v.value<QModelIndex>();
                    args[i]._Pointer = makeqtype<QModelIndexType>((Context*)c, o, "qt.QModelIndex");
                }
                break;

                case ItemSelectionArg:
                {
                    QMetaType type = QMetaType(signalArgTypes().at(i));
                    QVariant v(type, a[i + 1]);
                    QItemSelection o = v.value<QItemSelection>();
                    args[i]._Pointer = makeqtype<QItemSelectionType>((Context*)c, o, "qt.QItemSelection");
                }
                break;

                default:
                    failed = true;
                    cout << "SignalSpy::qt_metacall: don't know type: " << _argTypes[i] << endl;
                    break;
                }
            }

            if (failed)
                return methodId;

            if (_env)
            {
                _env->call(_F, args);
            }
            else
            {
                Thread* thread = _process->newApplicationThread();
                _process->call(thread, _F, args);
                _process->releaseApplicationThread(thread);
            }
        }

        return methodId;
    }

//  hijack qt_metacall by using a filtered version of the moc file
//  that has the function renamed to original_qt_metacall
#include <MuQt6/generated/moc_SignalSpy_filtered.hpp>
} // namespace Mu
