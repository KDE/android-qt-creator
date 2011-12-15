TEMPLATE = app
TARGET = simple_test_app
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .

SOURCES +=  simple_test_app.cpp

QT += network
QT += script
QT += xml
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += core-private
    QT *= widgets
}
#unix: QMAKE_CXXFLAGS += -msse2
#DEFINES += USE_BOOST=1

message("this says <foo & bar>")

maemo5 {
    target.path = /opt/usr/lib
    target.path = /opt
    INSTALLS += target
}

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += USE_PRIVATE
exists(/usr/include/boost/optional.hpp): DEFINES += USE_BOOST
exists(/usr/include/eigen2/Eigen/Core): DEFINES += USE_EIGEN

# Use for semi-automated testing
#DEFINES += USE_AUTORUN=1
