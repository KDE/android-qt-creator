/****************************************************************************
**
** Copyright (C) 2007 Trolltech ASA. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "../shared/shared.h"

int main(int argc, char **argv)
{
    if (argc != 3) {
        qDebug() << "Changeqt changes witch qt frameworks an application links against.";
        qDebug() << "Usage: changeqt app-bundle qt-dir";
        return 0;
    }
    
    const QString appPath = QString::fromLocal8Bit(argv[1]);
    const QString qtPath = QString::fromLocal8Bit(argv[2]);
    changeQtFrameworks(appPath, qtPath);
}
