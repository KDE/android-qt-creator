#!/bin/bash

if [ "$1" = "" -o "$2" = "" ]; then
	echo "$0 :: Error. Pass in release or debug then the install folder"
	echo "e.g. ./build.sh release C:/Necessitas/qtcreator-2.1.0"
	exit 1
fi

if [ "$2" = "release" ]; then
        QTDIR=C:/Qt/4.7.2-Git-MinGW-ShXcR
else
        QTDIR=C:/Qt/4.7.2-Git-MinGW-ShXcD
fi

# Using C:\Qt\4.7.1 doesn't work, though I should make this check the host os first, and use string substitution.
INCLUDE="-I$QTDIR/include"
PATH=$QTDIR/bin:$PATH
# Mixed paths don't work with qmake (i.e. arg0 is always used to find the directory and it's always a Windows path).
echo $QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/private-headers/include" "QT_CONFIG=webkit $1 svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r `dirname $0`/qtcreator.pro
$QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/private-headers/include" "QT_CONFIG=webkit $1 svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r `dirname $0`/qtcreator.pro

# Without this, make will not be able to translate relative paths
# properly as it can't step beyond where / is mounted.
if [ "$OSTYPE" = "msys" ]; then
	MAKEDIR=`pwd -W`
	MAKEFILE=$MAKEDIR/Makefile
else
	MAKEFILE=Makefile
fi


make -f $MAKEFILE -j9 $1
while [ "$?" != "0" ]
do
	if [ -f /usr/break-make ]; then
		echo "Detected break-make"
		rm -f /usr/break-make
		exit 1
	fi
	make -f $MAKEFILE -j9 $1
done

# `dirname $0`/copy-to-qt-dir.sh $2 $QTDIR
