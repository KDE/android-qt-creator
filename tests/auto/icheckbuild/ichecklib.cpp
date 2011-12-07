/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "ichecklib.h"
#include "parsemanager.h"
#include <QtCore/QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

QStringList getQTIncludePath()
{
    QStringList ret;
    QStringList processevironment = QProcess::systemEnvironment();
    foreach(QString item, processevironment){
        if(item.indexOf("QTDIR=") == 0){
            QString qtpath = item.remove("QTDIR=");
            ret << qtpath + "/include";
            ret << qtpath + "/include/ActiveQt";
            ret << qtpath + "/include/phonon";
            ret << qtpath + "/include/phonon_compat";
            ret << qtpath + "/include/Qt";
            ret << qtpath + "/include/Qt3Support";
            ret << qtpath + "/include/QtAssistant";
            ret << qtpath + "/include/QtCore";
            ret << qtpath + "/include/QtDBus";
            ret << qtpath + "/include/QtDeclarative";
            ret << qtpath + "/include/QtDesigner";
            ret << qtpath + "/include/QtGui";
            ret << qtpath + "/include/QtHelp";
            ret << qtpath + "/include/QtMultimedia";
            ret << qtpath + "/include/QtNetwork";
            ret << qtpath + "/include/QtOpenGL";
            ret << qtpath + "/include/QtOpenVG";
            ret << qtpath + "/include/QtScript";
            ret << qtpath + "/include/QtScriptTools";
            ret << qtpath + "/include/QtSql";
            ret << qtpath + "/include/QtSvg";
            ret << qtpath + "/include/QtTest";
            ret << qtpath + "/include/QtUiTools";
            ret << qtpath + "/include/QtWebKit";
            ret << qtpath + "/include/QtXml";
            ret << qtpath + "/include/QtXmlPatterns";
            break;
        }
    }
    return ret;
}

ICheckLib::ICheckLib()
: pParseManager(0)
{
}

void ICheckLib::ParseHeader(const QStringList& includePath, const QStringList& filelist)
{
    if(pParseManager)
        delete pParseManager;
    pParseManager = 0;
    pParseManager = new CPlusPlus::ParseManager();
    pParseManager->setIncludePath(includePath);
    pParseManager->parse(filelist);
}

bool ICheckLib::check(const ICheckLib& ichecklib /*ICheckLib from interface header*/, QString outputfile)
{
    if(pParseManager){
        CPlusPlus::ParseManager* cpparsemanager = ichecklib.pParseManager;
        return pParseManager->checkAllMetadatas(cpparsemanager, outputfile);
    }
    return false;
}

QStringList ICheckLib::getErrorMsg()
{
    QStringList ret;
    if(pParseManager){
        ret.append(pParseManager->getErrorMsg());
    }
    else
        ret << "no file was parsed.";
    return ret;
}
