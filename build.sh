#!/bin/bash

CLEAN=1
CONFIGURE=1
BUILDDEBUG=0
BUILDRELEASE=1
DEST_DIR=C:/Necessitas/qtcreator-2.1.0

while getopts "p:c:q:d:r:o:" arg; do
	echo $arg $OPTARG
	case $arg in
		p)
			echo "Usage :: ./build.sh -r 1 -d 0 -c 1 -q 1 -o C:/Necessitas/qtcreator-2.1.0"
			exit 0
			;;
		c)
			CLEAN=$OPTARG
			;;
		q)
			CONFIGURE=$OPTARG
			;;
		r)
			BUILDRELEASE=$OPTARG
			;;
		d)
			BUILDDEBUG=$OPTARG
			;;
		o)
			DEST_DIR=$OPTARG
			;;
	esac
done

if [ "$CLEAN" == "1" ]; then
	CONFIGURE=1
fi

if [ "$BUILDDEBUG" = "1" -a "$BUILDRELEASE" = "1" ]; then
	echo "Can't build both debug and release QtCreator at the same time"
	exit 1
fi

if [ "$BUILDRELEASE" = "1" ]; then
	QTDIR=C:/Qt/4.7.2-Git-MinGW-ShXcR
	CONFIGTYPE="release"
else
	QTDIR=C:/Qt/4.7.2-Git-MinGW-ShXcD
	DEST_DIR=$DEST_DIR-dbg
	CONFIGTYPE="debug"
fi

# Without this, make will not be able to translate relative paths
# properly as it can't step beyond where / is mounted.
if [ "$OSTYPE" = "msys" ]; then
	MAKEDIR=`pwd -W`
	MAKEFILE=$MAKEDIR/Makefile
else
	MAKEFILE=Makefile
fi

if [ "$CLEAN" -eq "1" ]; then
	if [ -f Makefile ]; then
		make -f $MAKEFILE distclean
	fi
	if [ -d qmake ]; then
		pushd .
		cd qmake
		if [ -f Makefile ]; then
			make -f $MAKEFILE clean
		fi
		popd
	fi
fi

# Using C:\Qt\4.7.1 doesn't work, though I should make this check the host os first, and use string substitution.
INCLUDE="-I$QTDIR/include"
# Mixed paths don't work with qmake (i.e. arg0 is always used to find the directory and it's always a Windows path).
if [ "$CONFIGURE" = "1" ]; then
	echo $QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/private-headers/include" "QT_CONFIG=webkit $CONFIGTYPE svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r `dirname $0`/qtcreator.pro
	$QTDIR/bin/qmake.exe "QT_PRIVATE_HEADERS=$QTDIR/private-headers/include" "QT_CONFIG=webkit $CONFIGTYPE svg declarative" -spec $QTDIR/mkspecs/win32-g++ -r `dirname $0`/qtcreator.pro
fi

make -f $MAKEFILE -j9 $CONFIGTYPE
while [ "$?" != "0" ]
do
	if [ -f /usr/break-make ]; then
		echo "Detected break-make"
		rm -f /usr/break-make
		exit 1
	fi
	make -f $MAKEFILE -j9 $CONFIGTYPE
done

`dirname $0`/copy-to-qt-dir.sh $DEST_DIR C:/Qt/qtcreator-2.1.0 $QTDIR C:/Qt/4.7.2-official
