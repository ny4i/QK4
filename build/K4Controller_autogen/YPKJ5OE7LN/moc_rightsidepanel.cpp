/****************************************************************************
** Meta object code from reading C++ file 'rightsidepanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/rightsidepanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rightsidepanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14RightSidePanelE_t {};
} // unnamed namespace

template <> constexpr inline auto RightSidePanel::qt_create_metaobjectdata<qt_meta_tag_ZN14RightSidePanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "RightSidePanel",
        "preClicked",
        "",
        "nbClicked",
        "nrClicked",
        "ntchClicked",
        "filClicked",
        "abClicked",
        "revClicked",
        "atobClicked",
        "spotClicked",
        "modeClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'preClicked'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'nbClicked'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'nrClicked'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'ntchClicked'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'filClicked'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'abClicked'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'revClicked'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'atobClicked'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'spotClicked'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'modeClicked'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<RightSidePanel, qt_meta_tag_ZN14RightSidePanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject RightSidePanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RightSidePanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RightSidePanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14RightSidePanelE_t>.metaTypes,
    nullptr
} };

void RightSidePanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RightSidePanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->preClicked(); break;
        case 1: _t->nbClicked(); break;
        case 2: _t->nrClicked(); break;
        case 3: _t->ntchClicked(); break;
        case 4: _t->filClicked(); break;
        case 5: _t->abClicked(); break;
        case 6: _t->revClicked(); break;
        case 7: _t->atobClicked(); break;
        case 8: _t->spotClicked(); break;
        case 9: _t->modeClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::preClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::nbClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::nrClicked, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::ntchClicked, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::filClicked, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::abClicked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::revClicked, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::atobClicked, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::spotClicked, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (RightSidePanel::*)()>(_a, &RightSidePanel::modeClicked, 9))
            return;
    }
}

const QMetaObject *RightSidePanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RightSidePanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RightSidePanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int RightSidePanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void RightSidePanel::preClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RightSidePanel::nbClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void RightSidePanel::nrClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void RightSidePanel::ntchClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void RightSidePanel::filClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void RightSidePanel::abClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void RightSidePanel::revClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void RightSidePanel::atobClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void RightSidePanel::spotClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void RightSidePanel::modeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}
QT_WARNING_POP
