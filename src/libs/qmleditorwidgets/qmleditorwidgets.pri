INCLUDEPATH += $$PWD $$PWD/easingpane
QT += declarative

LIBS *= -l$$qtLibraryName(QmlEditorWidgets)

include(../qmljs/qmljs.pri)
