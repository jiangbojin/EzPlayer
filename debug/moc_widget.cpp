/****************************************************************************
** Meta object code from reading C++ file 'widget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../widget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'widget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Widget_t {
    QByteArrayData data[25];
    char stringdata0[199];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Widget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Widget_t qt_meta_stringdata_Widget = {
    {
QT_MOC_LITERAL(0, 0, 6), // "Widget"
QT_MOC_LITERAL(1, 7, 10), // "playFinish"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 7), // "setYuyv"
QT_MOC_LITERAL(4, 27, 4), // "yuyv"
QT_MOC_LITERAL(5, 32, 5), // "clear"
QT_MOC_LITERAL(6, 38, 12), // "setFrameSize"
QT_MOC_LITERAL(7, 51, 5), // "width"
QT_MOC_LITERAL(8, 57, 6), // "height"
QT_MOC_LITERAL(9, 64, 14), // "updateTextures"
QT_MOC_LITERAL(10, 79, 7), // "quint8*"
QT_MOC_LITERAL(11, 87, 5), // "dataY"
QT_MOC_LITERAL(12, 93, 5), // "dataU"
QT_MOC_LITERAL(13, 99, 5), // "dataV"
QT_MOC_LITERAL(14, 105, 9), // "linesizeY"
QT_MOC_LITERAL(15, 115, 9), // "linesizeU"
QT_MOC_LITERAL(16, 125, 9), // "linesizeV"
QT_MOC_LITERAL(17, 135, 6), // "dataUV"
QT_MOC_LITERAL(18, 142, 10), // "linesizeUV"
QT_MOC_LITERAL(19, 153, 11), // "updateFrame"
QT_MOC_LITERAL(20, 165, 4), // "read"
QT_MOC_LITERAL(21, 170, 4), // "play"
QT_MOC_LITERAL(22, 175, 8), // "fileName"
QT_MOC_LITERAL(23, 184, 9), // "frameRate"
QT_MOC_LITERAL(24, 194, 4) // "stop"

    },
    "Widget\0playFinish\0\0setYuyv\0yuyv\0clear\0"
    "setFrameSize\0width\0height\0updateTextures\0"
    "quint8*\0dataY\0dataU\0dataV\0linesizeY\0"
    "linesizeU\0linesizeV\0dataUV\0linesizeUV\0"
    "updateFrame\0read\0play\0fileName\0frameRate\0"
    "stop"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Widget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   65,    2, 0x0a /* Public */,
       5,    0,   68,    2, 0x0a /* Public */,
       6,    2,   69,    2, 0x0a /* Public */,
       9,    6,   74,    2, 0x0a /* Public */,
       9,    4,   87,    2, 0x0a /* Public */,
      19,    8,   96,    2, 0x0a /* Public */,
      20,    0,  113,    2, 0x08 /* Private */,
      21,    2,  114,    2, 0x0a /* Public */,
      24,    0,  119,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    7,    8,
    QMetaType::Void, 0x80000000 | 10, 0x80000000 | 10, 0x80000000 | 10, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt,   11,   12,   13,   14,   15,   16,
    QMetaType::Void, 0x80000000 | 10, 0x80000000 | 10, QMetaType::UInt, QMetaType::UInt,   11,   17,   14,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, 0x80000000 | 10, 0x80000000 | 10, 0x80000000 | 10, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt,    7,    8,   11,   12,   13,   14,   15,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   22,   23,
    QMetaType::Void,

       0        // eod
};

void Widget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Widget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->playFinish(); break;
        case 1: _t->setYuyv((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->clear(); break;
        case 3: _t->setFrameSize((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->updateTextures((*reinterpret_cast< quint8*(*)>(_a[1])),(*reinterpret_cast< quint8*(*)>(_a[2])),(*reinterpret_cast< quint8*(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4])),(*reinterpret_cast< quint32(*)>(_a[5])),(*reinterpret_cast< quint32(*)>(_a[6]))); break;
        case 5: _t->updateTextures((*reinterpret_cast< quint8*(*)>(_a[1])),(*reinterpret_cast< quint8*(*)>(_a[2])),(*reinterpret_cast< quint32(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4]))); break;
        case 6: _t->updateFrame((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< quint8*(*)>(_a[3])),(*reinterpret_cast< quint8*(*)>(_a[4])),(*reinterpret_cast< quint8*(*)>(_a[5])),(*reinterpret_cast< quint32(*)>(_a[6])),(*reinterpret_cast< quint32(*)>(_a[7])),(*reinterpret_cast< quint32(*)>(_a[8]))); break;
        case 7: _t->read(); break;
        case 8: _t->play((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 9: _t->stop(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Widget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::playFinish)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Widget::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_Widget.data,
    qt_meta_data_Widget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Widget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Widget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Widget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QOpenGLFunctions"))
        return static_cast< QOpenGLFunctions*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int Widget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void Widget::playFinish()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
