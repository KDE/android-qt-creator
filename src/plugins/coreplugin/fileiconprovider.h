/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <coreplugin/core_global.h>

#include <QtGui/QStyle>
#include <QtGui/QFileIconProvider>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QIcon;
class QPixmap;
class QString;
QT_END_NAMESPACE

namespace Core {

class MimeType;
struct FileIconProviderPrivate;

class CORE_EXPORT FileIconProvider : public QFileIconProvider
{
    Q_DISABLE_COPY(FileIconProvider)
    FileIconProvider();

public:
    virtual ~FileIconProvider();

    // Implement QFileIconProvider
    virtual QIcon icon(const QFileInfo &info) const;
    using QFileIconProvider::icon;

    // Register additional overlay icons
    static QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size);
    void registerIconOverlayForSuffix(const QIcon &icon, const QString &suffix);
    void registerIconOverlayForMimeType(const QIcon &icon, const MimeType &mimeType);

    static FileIconProvider *instance();

private:
    FileIconProviderPrivate *d;
};

} // namespace Core

#endif // FILEICONPROVIDER_H
