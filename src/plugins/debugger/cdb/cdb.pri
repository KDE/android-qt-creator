HEADERS += $$PWD/cdbengine.h \
    cdb/bytearrayinputstream.h \
    cdb/cdbparsehelpers.h \
    cdb/cdboptions.h \
    cdb/cdboptionspage.h

SOURCES += $$PWD/cdbengine.cpp \
    cdb/bytearrayinputstream.cpp \
    cdb/cdbparsehelpers.cpp \
    cdb/cdboptions.cpp \
    cdb/cdboptionspage.cpp

FORMS += cdb/cdboptionspagewidget.ui

INCLUDEPATH*=$$PWD
