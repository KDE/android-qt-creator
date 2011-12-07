/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSet>
#include <QMap>
#include <QProcess>
#include <QTextStream>
#include <QVector>

//
// Command line options
//

// Put everything into a single project.
static int targetDepth = 0;
static bool forceOverWrite = false;
static QString subProjectSeparator = QString(QLatin1Char('_'));

// FIXME: Make file classes configurable on the command line.
static const char *defaultExtensions[] =
{
    "SOURCES",      "cpp,c,C,cxx,c++",
    "HEADERS",      "hpp,h,H,hxx,h++",
    "TRANSLATIONS", "ts",
    "FORMS",        "ui",
};

typedef QHash<QByteArray, QByteArray> Extensions;

static QByteArray proExtension = "pro";


class FileClass
{
public:
    FileClass() {}

    //! suffixes is a comma separated string of extensions.
    FileClass(const QByteArray &suffixes, const QString &varName)
        : m_suffixes(',' + suffixes + ','), m_varName(varName)
    {}

    static QByteArray prepareSuffix(const QByteArray &suffix)
    {
        return ',' + suffix + ',';
    }

    bool canHandle(const QByteArray &preparedSuffix) const
    {
        return m_suffixes.contains(preparedSuffix);
    }

    void addFile(const QFileInfo &fi)
    {
        m_files.insert(fi.filePath(), Dummy());
    }

    bool handleFile(const QFileInfo &fi, const QByteArray &preparedSuffix)
    {
        if (!canHandle(preparedSuffix))
            return false;
        addFile(fi);
        return true;
    }

    void writeProBlock(QTextStream &ts) const
    {
        if (m_files.isEmpty())
            return;
        ts << '\n' << m_varName << " *=";
        Files::ConstIterator it = m_files.begin();
        Files::ConstIterator end = m_files.end();
        for ( ; it != end; ++it)
            ts << " \\\n    " << it.key();
        ts << "\n";
    }

private:
    struct Dummy {};
    typedef QMap<QString, Dummy> Files;

    QByteArray m_suffixes;
    QString m_varName;
    Files m_files;
};

class ProMaker;

class Project
{
    friend class ProMaker;
    explicit Project(ProMaker *master)
      : m_master(master)
    {}

public:
    void setTreeLevel(int level) { m_treeLevel = level; }
    int treeLevel() const { return m_treeLevel; }
    void setPaths(const QStringList &paths);
    void setOutputFileName(const QString &fileName) { m_outputFileName = fileName; }
    void writeProFile();
    void createFileLists();
    void setExtensions(const Extensions &extensions);
    Extensions extensions() const { return m_extensions; }

private:
    void handleItem(const QString &item);
    void handleDir(const QString &item);
    void handleBinary(const QString &item);

    ProMaker *m_master;
    QStringList m_items;
    QVector<FileClass> m_fileClasses;
    Extensions m_extensions;
    QString m_outputFileName;
    QStringList m_subdirs;
    int m_treeLevel;
};

class ProMaker
{
public:
    ProMaker() {}
    ~ProMaker() { qDeleteAll(m_projects); }
    Project *addEmptyProject();
    void createContents();
    void writeOutput();

private:
    QVector<Project *> m_projects;
};

void Project::setPaths(const QStringList &paths)
{
    foreach (const QString &path, paths)
        m_items.append(path);
}

void Project::setExtensions(const Extensions &extensions)
{
    m_fileClasses.clear();
    m_extensions = extensions;
    Extensions::ConstIterator it = extensions.begin();
    for ( ; it != extensions.end(); ++it)
        m_fileClasses.append(FileClass(it.value(), it.key()));
}

void Project::createFileLists()
{
    for (int i = 0; i != m_items.size(); ++i)
        handleItem(m_items.at(i));
}

void Project::handleItem(const QString &item)
{
    QFileInfo fi(item);
    if (fi.isDir())
        handleDir(item);
    else
        handleBinary(item);
}

void Project::handleBinary(const QString &item)
{
    QStringList args;
    args.append("--batch-silent");
    args.append("--nx");
    args.append("--quiet");
    args.append("-i");
    args.append("mi");
    args.append("--se=" + item);
    args.append("-ex");
    args.append("interpreter-exec mi -file-list-exec-source-files");
    args.append("-ex");
    args.append("quit");
    QProcess proc;
    proc.start("gdb", args);
    if (!proc.waitForStarted()) {
        qDebug() << "COULD NOT START";
        return;
    }
    if (!proc.waitForFinished()) {
        qDebug() << "COULD NOT FINISH";
        return;
    }
    QByteArray ba = proc.readAllStandardOutput();
    if (ba.isEmpty()) {
        qDebug() << "NO OUTPUT";
        return;
    }
    QString input = QString::fromLatin1(ba, ba.size());
    // ^done,files=[{file="<<C++-namespaces>>",{file=...,fullname=}
    // "}] (gdb)
    int first = input.indexOf('{');
    input = input.mid(first, input.lastIndexOf('}') - first);
    foreach (QString item, input.split("},{")) {
        //qDebug() << "ITEM: " << item;
        int full = item.indexOf(",fullname=\"");
        if (full != -1)
            item = item.mid(full + 11);
        else
            item = item.mid(6);
        item.chop(1);
        //qDebug() << "ITEM: " << item;
        QFileInfo fi(item);
        const QByteArray ext = FileClass::prepareSuffix(fi.suffix().toUtf8());
        for (int i = m_fileClasses.size(); --i >= 0; ) {
            if (m_fileClasses[i].handleFile(fi, ext))
                break;
        }
    }
}

void Project::handleDir(const QString &item)
{
    QDirIterator it(item);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (fi.isDir()) {
            if (fi.fileName() == ".." || fi.fileName() == ".")
                continue;
            const QString filePath = fi.filePath();
            if (m_treeLevel < targetDepth) {
                QString subName = m_outputFileName;
                subName.insert(subName.size() - 1 - proExtension.size(),
                    subProjectSeparator + fi.fileName());
                Project *child = m_master->addEmptyProject();
                child->setTreeLevel(treeLevel() + 1);
                child->setPaths(QStringList(filePath));
                child->setOutputFileName(subName);
                child->setExtensions(extensions());
                m_subdirs.append(subName);
            } else {
                m_items.append(fi.filePath());
            }
        } else {
            const QByteArray ext = FileClass::prepareSuffix(fi.suffix().toUtf8());
            for (int i = m_fileClasses.size(); --i >= 0; ) {
                if (m_fileClasses[i].handleFile(fi, ext))
                    break;
            }
        }
    }
}

void Project::writeProFile()
{
    QFile file(m_outputFileName);
    if (file.exists()) {
        if (!forceOverWrite) {
            qWarning("%s", qPrintable(QString(
                "%1 not overwritten. Use -f if this should be done.")
                .arg(m_outputFileName)));
            return;
        }
        if (!file.remove()) {
            qWarning("%s", qPrintable(QString(
                "%1 could not be deleted.").arg(m_outputFileName)));
            return;
        }
    }

    if (!file.open(QIODevice::WriteOnly)) {
         qWarning("%s", qPrintable(QString(
            "%1 cannot be written").arg(m_outputFileName)));
         return;
    }


    QTextStream ts(&file);
    ts << "#####################################################################\n";
    ts << "# Automatically generated by qtpromaker\n";
    ts << "#####################################################################\n\n";

    if (m_subdirs.isEmpty()) {
        ts << "TEMPLATE = app\n";
        ts << "TARGET = " << QFileInfo(m_outputFileName).baseName() << "\n";
        foreach (const FileClass &fc, m_fileClasses)
            fc.writeProBlock(ts);
        ts << "\nPATHS *=";
        foreach (const QDir &dir, m_items)
            ts << " \\\n    " << dir.path();
        ts << "\n\nDEPENDPATH *= $$PATHS\n";
        ts << "\nINCLUDEPATH *= $$PATHS\n";
    } else {
        ts << "TEMPLATE = subdirs\n";
        ts << "SUBDIRS = ";
        foreach (const QString &subdir, m_subdirs)
            ts << " \\\n    " << subdir;
        ts << "\n";
    }
}

void ProMaker::createContents()
{
    for (int i = 0; i != m_projects.size(); ++i)
        m_projects[i]->createFileLists();
}

void ProMaker::writeOutput()
{
    for (int i = 0; i != m_projects.size(); ++i)
        m_projects.at(i)->writeProFile();
}

Project *ProMaker::addEmptyProject()
{
    Project *project = new Project(this);
    m_projects.append(project);
    return project;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    // Set up default values.
    QStringList paths;
    Extensions extensions;
    int extensionsCount = sizeof(defaultExtensions)/sizeof(defaultExtensions[0]);
    for (int i = 0; i < extensionsCount; i += 2)
        extensions[defaultExtensions[i]] = defaultExtensions[i + 1];

    QString outputFileName;

    // Override by command line.
    for (int i = 1, n = args.size(); i < n; ++i) {
        const QString arg = args.at(i);
        if (arg == "-h" || arg == "--help" || arg == "-help") {
            qWarning() << "Usage: " << qPrintable(args.at(0))
                << " [-f] [-o out.pro] [dir...]"
                << "\n\n"
                << "Arguments:\n"
                << "  -f, --force             overwrite existing files\n"
                << "  -d, --depth <n>         recursion depth for sub-projects\n"
                << "  -s, --separator <char>  separator for sub-project names\n"
                << "  -o, --output <file>     output to <file>\n"
                << "  -h, --help              print this help\n";
            return 1;
        }

        bool handled = true;
        if (arg == "-f" || arg == "--force" || arg == "-force") {
            forceOverWrite = true;
        } else if (i < n - 1) {
            if (arg == "-o" || arg == "--output" || arg == "-output")
                outputFileName = args.at(++i);
            else if (arg == "-d" || arg == "--depth" || arg == "-depth")
                targetDepth = args.at(++i).toInt();
            else if (arg == "-s" || arg == "--separator" || arg == "-separator")
                subProjectSeparator = args.at(++i);
            else
                handled = false;
        }

        // Nothing know. Treat it as path.
        if (!handled)
            paths.append(args.at(i));
    }


    // Fallbacks.
    if (paths.isEmpty()) {
        QDir dir = QDir::currentPath();
        outputFileName = dir.dirName() + '.' + proExtension;
        paths.append(".");
    }

    if (outputFileName.isEmpty()) {
        outputFileName = QDir(paths.at(0)).dirName() + '.' + proExtension;
    }

    if (targetDepth == -1)
        targetDepth = 1000000; // "infinity"

    //qDebug() << "DEPTH: " << targetDepth;
    //qDebug() << "SEPARATOR: " << subProjectSeparator;

    // Run the thing.
    ProMaker pm;

    Project *p = pm.addEmptyProject();
    p->setExtensions(extensions);
    p->setPaths(paths);
    p->setOutputFileName(outputFileName);

    pm.createContents();
    pm.writeOutput();

    return 0;
}
