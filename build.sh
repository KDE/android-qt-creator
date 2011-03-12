#!/bin/bash

# Pass in release or debug then the install folder.

# Using C:\Qt\4.7.1 doesn't work, though I should make this check the host os first, and use string substitution.
QTDIR="C:/Qt/4.7.2-Git-MinGW-ShXcR"
INCLUDE="-I$QTDIR/include"
PATH=$QTDIR/bin:$PATH
$QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/private-headers/include" "QT_CONFIG=webkit $1 svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r `dirname $0`/qtcreator.pro

make -j9 $1
while [ "$?" != "0" ]
do
	if [ -f /usr/break-make ]; then
		echo "Detected break-make"
		rm -f /usr/break-make
		exit 1
	fi
	make -j9 $1
done

`dirname $0`/copy-to-qt-dir.sh $2
