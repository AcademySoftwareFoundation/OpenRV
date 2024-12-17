//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__qtUtils__h__
#define __MuQt__qtUtils__h__
#include <iostream>
#include <gc/gc.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/MuLangContext.h>
#include <MuQt5/QObjectType.h>
#include <MuQt5/QPixmapType.h>
#include <MuQt5/QModelIndexType.h>
#include <MuQt5/QStandardItemType.h>
#include <MuQt5/QListWidgetItemType.h>
#include <MuQt5/QTreeWidgetItemType.h>
#include <MuQt5/QTableWidgetItemType.h>
#include <MuQt5/QPaintDeviceType.h>
#include <MuQt5/QLayoutItemType.h>
#include <MuQt5/QItemSelectionType.h>
#include <MuQt5/QFileInfoType.h>
#include <MuQt5/QNetworkCookieType.h>

Q_DECLARE_METATYPE(QTreeWidgetItem*);
Q_DECLARE_METATYPE(QListWidgetItem*);
Q_DECLARE_METATYPE(QTableWidgetItem*);
Q_DECLARE_METATYPE(QAction*);
Q_DECLARE_METATYPE(QStandardItem*);
Q_DECLARE_METATYPE(QModelIndex);
Q_DECLARE_METATYPE(QPixmap);
// Q_DECLARE_METATYPE(QItemSelection);
Q_DECLARE_METATYPE(Qt::DockWidgetArea);
Q_DECLARE_METATYPE(Qt::DockWidgetAreas);
Q_DECLARE_METATYPE(QDockWidget::DockWidgetFeatures);

typedef int MuQtPublicEnum;

namespace Mu
{

    bool isMuQtObject(QObject*);
    bool isMuQtLayoutItem(QLayoutItem*);
    bool isMuQtPaintDevice(QPaintDevice*);

    template <class T> T* object(Pointer obj)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            return dynamic_cast<T*>(i->data<Mu::QObjectType::Struct>()->object);
        else
            return 0;
    }

    inline void setobject(Pointer obj, const QObject* object)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            i->data<Mu::QObjectType::Struct>()->object = (QObject*)object;
    }

    template <class T> T* layoutitem(Pointer obj)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            return dynamic_cast<T*>(i->data<QLayoutItemType::Struct>()->object);
        else
            return 0;
    }

    inline void setlayoutitem(Pointer obj, const QLayoutItem* object)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            i->data<QLayoutItemType::Struct>()->object = (QLayoutItem*)object;
    }

    template <class T> T* paintdevice(Pointer obj)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            return dynamic_cast<T*>(
                i->data<QPaintDeviceType::Struct>()->object);
        else
            return 0;
    }

    inline void setpaintdevice(Pointer obj, const QPaintDevice* object)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            i->data<QPaintDeviceType::Struct>()->object = (QPaintDevice*)object;
    }

    inline void setpaintdevice(Pointer obj, const QPaintDevice& object)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            i->data<QPaintDeviceType::Struct>()->object =
                (QPaintDevice*)&object;
    }

    template <class Type>
    inline typename Type::ValueType* getqpointer(Pointer obj)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            return dynamic_cast<typename Type::ValueType*>(
                i->data<typename Type::Struct>()->object);
        else
            return 0;
    }

    template <class Type>
    inline void setqpointer(Pointer obj, const typename Type::ValueType* object)
    {
        if (ClassInstance* i = reinterpret_cast<ClassInstance*>(obj))
            i->data<typename Type::Struct>()->object =
                (typename Type::ValueType*)object;
    }

    template <class Type>
    inline ClassInstance* makeqpointer(Context* c,
                                       const typename Type::ValueType* object,
                                       const char* name)
    {
        if (!object)
            return 0;
        const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
            c->internName(name), false);
        ClassInstance* i = ClassInstance::allocate(type);
        setqpointer<Type>(i, (typename Type::ValueType*)object);
        return i;
    }

    template <class Type>
    inline ClassInstance* makeqpointer(const Type* type,
                                       const typename Type::ValueType* qobject)
    {
        if (!qobject)
            return 0;
        ClassInstance* i = ClassInstance::allocate(type);
        setqpointer<Type>(i, (typename Type::ValueType*)qobject);
        return i;
    }

    template <class Type>
    QList<typename Type::ValueType*> qpointerlist(Pointer p)
    {
        QList<typename Type::ValueType*> list;
        DynamicArray* array = reinterpret_cast<DynamicArray*>(p);
        for (size_t i = 0; i < array->size(); i++)
        {
            list.push_back(
                getqpointer<Type>(array->element<ClassInstance*>(i)));
        }

        return list;
    }

    template <class Type>
    DynamicArray* makeqpointerlist(Context* c,
                                   const QList<typename Type::ValueType*>& list,
                                   const char* name)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        const Class* t = lc->findSymbolOfTypeByQualifiedName<Class>(
            c->internName(name), false);
        DynamicArray* array = new DynamicArray(
            static_cast<const Class*>(lc->arrayType(t, 1, 0)), 1);
        array->resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            array->element<ClassInstance*>(i) =
                makeqpointer<Type>(c, list[i], name);
        }

        return array;
    }

    template <class MuType>
    ClassInstance* makeinstance(Context* c, const QObject* object,
                                const char* name)
    {
        const typename MuType::MuQtType* mobj = 0;

        if (MuType::isInheritable()
            && (mobj = dynamic_cast<const typename MuType::MuQtType*>(object)))
        {
            return MuType::cachedInstance(mobj);
        }
        else
        {
            const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
                c->internName(name), false);
            ClassInstance* i = ClassInstance::allocate(type);
            setobject(i, (QObject*)object);
            return i;
        }
    }

    template <class MuType>
    ClassInstance* makelayoutitem(Context* c, const QLayoutItem* item,
                                  const char* name)
    {
        const typename MuType::MuQtType* mobj = 0;

        if (MuType::isInheritable()
            && (mobj = dynamic_cast<const typename MuType::MuQtType*>(item)))
        {
            return MuType::cachedInstance(mobj);
        }
        else
        {
            const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
                c->internName(name), false);
            ClassInstance* i = ClassInstance::allocate(type);
            setlayoutitem(i, (QLayoutItem*)item);
            return i;
        }
    }

    template <class MuType>
    ClassInstance* makepaintdevice(Context* c, const QPaintDevice* device,
                                   const char* name)
    {
        const typename MuType::MuQtType* mobj = 0;

        if (MuType::isInheritable()
            && (mobj = dynamic_cast<const typename MuType::MuQtType*>(device)))
        {
            return MuType::cachedInstance(mobj);
        }
        else
        {
            const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
                c->internName(name), false);
            ClassInstance* i = ClassInstance::allocate(type);
            setpaintdevice(i, (QPaintDevice*)device);
            return i;
        }
    }

    template <class T>
    ClassInstance* makeinstance(const T* type, const QObject* qobject)
    {
        const typename T::MuQtType* mobj = 0;

        if (T::isInheritable()
            && (mobj = dynamic_cast<const typename T::MuQtType*>(qobject)))
        {
            return T::cachedInstance(mobj);
        }
        else
        {
            ClassInstance* i = ClassInstance::allocate(type);
            setobject(i, (QObject*)qobject);
            return i;
        }
    }

    template <class T>
    ClassInstance* makelayoutitem(const T* type, const QLayoutItem* qitem)
    {
        const typename T::MuQtType* mobj = 0;

        if (T::isInheritable()
            && (mobj = dynamic_cast<const typename T::MuQtType*>(qitem)))
        {
            return T::cachedInstance(mobj);
        }
        else
        {
            ClassInstance* i = ClassInstance::allocate(type);
            setlayoutitem(i, (QLayoutItem*)qitem);
            return i;
        }
    }

    template <class T>
    ClassInstance* makepaintdevice(const T* type, const QPaintDevice* device)
    {
        const typename T::MuQtType* mobj = 0;

        if (T::isInheritable()
            && (mobj = dynamic_cast<const typename T::MuQtType*>(device)))
        {
            return T::cachedInstance(mobj);
        }
        else
        {
            ClassInstance* i = ClassInstance::allocate(type);
            setpaintdevice(i, (QPaintDevice*)device);
            return i;
        }
    }

    inline QString qstring(Pointer p)
    {
        if (!p)
            return QString();
        return QString::fromUtf8(
            reinterpret_cast<StringType::String*>(p)->c_str());
    }

    inline StringType::String* makestring(Context* c, const QString& qstr)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        return lc->stringType()->allocate(qstr.toUtf8().constData());
    }

    inline StringType::String* makestring(Context* c, const char* str)
    {
        if (str)
        {
            MuLangContext* lc = static_cast<MuLangContext*>(c);
            return lc->stringType()->allocate(str);
        }
        else
        {
            return 0;
        }
    }

    inline QStringList qstringlist(Pointer p)
    {
        QStringList list;
        DynamicArray* array = reinterpret_cast<DynamicArray*>(p);
        for (size_t i = 0; i < array->size(); i++)
        {
            list.push_back(
                array->element<const StringType::String*>(i)->c_str());
        }

        return list;
    }

    inline DynamicArray* makestringlist(Context* c, const QStringList& list)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        DynamicArray* array = new DynamicArray(
            static_cast<const Class*>(lc->arrayType(lc->stringType(), 1, 0)),
            1);
        array->resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            array->element<StringType::String*>(i) = makestring(c, list[i]);
        }

        return array;
    }

    template <class Type> inline typename Type::ValueType& getqtype(Pointer p)
    {
        return reinterpret_cast<typename Type::Instance*>(p)->value;
    }

    template <class Type>
    inline void setqtype(Pointer p, const typename Type::ValueType& value)
    {
        reinterpret_cast<typename Type::Instance*>(p)->value = value;
    }

    template <class Type>
    inline ClassInstance* makeqtype(const Class* type,
                                    const typename Type::ValueType& value)
    {
        typename Type::Instance* i = new typename Type::Instance(type);
        Type::registerFinalizer(i);
        setqtype<Type>(i, value);
        return i;
    }

    template <class Type>
    inline ClassInstance* makeqtype(Context* c,
                                    const typename Type::ValueType& value,
                                    const char* muType)
    {
        const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
            c->internName(muType), false);
        typename Type::Instance* i = new typename Type::Instance(type);
        Type::registerFinalizer(i);
        setqtype<Type>(i, value);
        return i;
    }

    template <class QtType, class MuType>
    inline QList<QtType> qtypelist(Pointer p)
    {
        QList<QtType> list;
        DynamicArray* array = reinterpret_cast<DynamicArray*>(p);
        for (size_t i = 0; i < array->size(); i++)
        {
            list.push_back(
                array->element<typename MuType::Instance*>(i)->value);
        }

        return list;
    }

    template <class QtType, class MuType>
    inline DynamicArray* makeqtypelist(Context* c, const QList<QtType>& list,
                                       const char* muType)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        const Class* type = c->findSymbolOfTypeByQualifiedName<Class>(
            lc->internName(muType), false);
        DynamicArray* array = new DynamicArray(
            static_cast<const Class*>(lc->arrayType(type, 1, 0)), 1);
        array->resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            array->element<ClassInstance*>(i) =
                makeqtype<MuType>(type, list[i]);
        }

        return array;
    }

    Pointer assertNotNil(Thread& thread, const Node& node, Pointer p, size_t n);

    inline QModelIndexList qmodelindexlist(Pointer p)
    {
        QModelIndexList list;
        DynamicArray* array = reinterpret_cast<DynamicArray*>(p);
        for (size_t i = 0; i < array->size(); i++)
        {
            list.push_back(
                array->element<QModelIndexType::Instance*>(i)->value);
        }

        return list;
    }

    inline DynamicArray* makeqmodelindexlist(Context* c,
                                             const QModelIndexList& list)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        DynamicArray* array = new DynamicArray(
            static_cast<const Class*>(lc->arrayType(lc->stringType(), 1, 0)),
            1);
        array->resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            array->element<Pointer>(i) =
                makeqtype<QModelIndexType>(c, list[i], "qt.QModelIndex");
        }

        return array;
    }

    inline QFileInfoList qfileinfolist(Pointer p)
    {
        QFileInfoList list;
        DynamicArray* array = reinterpret_cast<DynamicArray*>(p);
        for (size_t i = 0; i < array->size(); i++)
        {
            list.push_back(array->element<QFileInfoType::Instance*>(i)->value);
        }

        return list;
    }

    inline DynamicArray* makeqfileinfolist(Context* c,
                                           const QFileInfoList& list)
    {
        MuLangContext* lc = static_cast<MuLangContext*>(c);
        DynamicArray* array = new DynamicArray(
            static_cast<const Class*>(lc->arrayType(lc->stringType(), 1, 0)),
            1);
        array->resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            array->element<Pointer>(i) =
                makeqtype<QFileInfoType>(c, list[i], "qt.QFileInfo");
        }

        return array;
    }

#define NONNIL_NODE_ARG(N, T) \
    assertNotNil(NODE_THREAD, NODE_THIS, NODE_ARG(N, T), N)

    template <typename T> inline T defaultValue() { return T(0); }

    template <> inline QModelIndex defaultValue<>() { return QModelIndex(); }

    template <> inline QRegion defaultValue<>() { return QRegion(); }

    template <> inline QRect defaultValue<>() { return QRect(); }

    template <> inline QSize defaultValue<>() { return QSize(); }

    Class::ClassVector vectorOf2(Class* a, Class* b);

    // NODE_DECLARATION(__allocate_register_gc_qobject, Pointer);

} // namespace Mu

#endif // __MuQt__qtUtils__h__
