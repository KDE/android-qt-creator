#!/bin/bash

echo $1
if [ "$1" != "" ] ; then
	DESTQTDIR=$1
fi

if [ "$DESTQTDIR" = "" ] ; then
	DESTQTDIR=$QTDIR
fi

echo Copying QtCreator to $DESTQTDIR

mkdir -p $DESTQTDIR/lib/qtcreator/plugins/Nokia
mkdir -p $DESTQTDIR/bin

cp bin/* $DESTQTDIR/bin
cp lib/qtcreator/*.dll $DESTQTDIR/lib/qtcreator
cp lib/qtcreator/plugins/Nokia/* $DESTQTDIR/lib/qtcreator/plugins/Nokia
