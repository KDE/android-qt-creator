INCLUDEPATH *= $$PWD

QT += network
win32:include(../../private_headers.pri)

# Input
HEADERS += $$PWD/symbianutils_global.h \
    $$PWD/callback.h \
    $$PWD/codautils.h \
    $$PWD/codautils_p.h \
    $$PWD/symbiandevicemanager.h \
    $$PWD/codadevice.h \
    $$PWD/codamessage.h \
    $$PWD/json.h \
    $$PWD/virtualserialdevice.h

SOURCES += $$PWD/codautils.cpp \
    $$PWD/symbiandevicemanager.cpp \
    $$PWD/codadevice.cpp \
    $$PWD/codamessage.cpp \
    $$PWD/json.cpp \
    $$PWD/virtualserialdevice.cpp

DEFINES += HAS_SERIALPORT
win32:SOURCES += $$PWD/virtualserialdevice_win.cpp
unix:SOURCES += $$PWD/virtualserialdevice_posix.cpp

macx:LIBS += -framework IOKit -framework CoreFoundation
