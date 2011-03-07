#!/bin/bash

# Using C:\Qt\4.7.1 doesn't work, though I should make this check the host os first, and use string substitution.
QTDIR="/Qt/4.7.1"
INCLUDE="-I$QTDIR/include"
PATH=$QTDIR/bin:$PATH
# "QT_INSTALL_HEADERS=$QTDIR/include"
# declarative is needed for the debugger plugin. webkit for help, svg for something too.

# PREFIX doesn't work?
# INSTALLLOC=/c/necessitas/QtCreator
# mkdir -p $INSTALLLOC
# $QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/qt/include" "QT_CONFIG=webkit release svg declarative" -spec $QTDIR/mkspecs/win32-g++ PREFIX=$INSTALLLOC -r ../mingw-android-qt-creator/qtcreator.pro
$QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/qt/include" "QT_CONFIG=webkit release svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r ../mingw-android-qt-creator/qtcreator.pro
make release -j5
