TEMPLATE = subdirs

include(../../../qtcreator.pri)
include(../../private_headers.pri)

exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    minQtVersion(4, 7, 1) {
        SUBDIRS += qmlpuppet
    }
}else {
    target.path  = /bin
    INSTALLS    += target
}
