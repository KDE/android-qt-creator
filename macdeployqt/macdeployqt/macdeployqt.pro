TEMPLATE = app
TARGET = macdeployqt
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp ../shared/shared.cpp
CONFIG += qt warn_on
CONFIG -= app_bundle

