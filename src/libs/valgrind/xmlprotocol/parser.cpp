/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "parser.h"
#include "announcethread.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "status.h"
#include "suppression.h"
#include <utils/qtcassert.h>

#include <QtNetwork/QAbstractSocket>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QPair>
#include <QtCore/QThread>
#include <QtCore/QXmlStreamReader>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

namespace {
    class Exception {
    public:
        explicit Exception(const QString &msg)
            : m_message(msg)
        {
        }

        ~Exception() throw() {}

        QString message() const
        {
            return m_message;
        }

    private:
        QString m_message;
    };

    class ParserException : public Exception {
    public:
        explicit ParserException(const QString &message)
            : Exception(message)
        {
        }

        ~ParserException() throw() {}
    };

    struct XWhat {
        XWhat() : leakedblocks(0), leakedbytes(0), hthreadid(-1) {}
        QString text;
        qint64 leakedblocks;
        qint64 leakedbytes;
        qint64 hthreadid;
    };

    struct XauxWhat {
        XauxWhat() : line(-1), hthreadid(-1) {}
        void clear()
        {
            *this = XauxWhat();
        }

        QString text;
        QString file;
        QString dir;
        qint64 line;
        qint64 hthreadid;
    };
}

class Parser::Private {
    Parser *const q;
public:

    explicit Private(Parser *qq);

    void parse(QIODevice *device);

    void parse_error();
    QVector<Frame> parse_stack();
    Suppression parse_suppression();
    SuppressionFrame parse_suppFrame();
    Frame parse_frame();
    void parse_status();
    void parse_errorcounts();
    void parse_suppcounts();
    void parse_announcethread();
    void checkProtocolVersion(const QString &versionStr);
    void checkTool(const QString &tool);
    XWhat parseXWhat();
    XauxWhat parseXauxWhat();
    MemcheckErrorKind parseMemcheckErrorKind(const QString &kind);
    HelgrindErrorKind parseHelgrindErrorKind(const QString &kind);
    PtrcheckErrorKind parsePtrcheckErrorKind(const QString &kind);
    int parseErrorKind(const QString &kind);

    void reportInternalError(const QString &errorString);
    QXmlStreamReader::TokenType blockingReadNext();
    bool notAtEnd() const;
    QString blockingReadElementText();

    Tool tool;
    QString errorString;
    QXmlStreamReader reader;
    QHash<QString,MemcheckErrorKind> errorKindsByName_memcheck;
    QHash<QString,HelgrindErrorKind> errorKindsByName_helgrind;
    QHash<QString,PtrcheckErrorKind> errorKindsByName_ptrcheck;
    QHash<QString,Parser::Tool> toolsByName;
};

#undef ADD_ENUM
#define ADD_ENUM(tool,enumV) { errorKindsByName_##tool.insert(QLatin1String(#enumV), enumV); }


Parser::Private::Private(Parser *qq)
    : q(qq),
      tool(Parser::Unknown)
{
    toolsByName.insert(QLatin1String("memcheck"), Parser::Memcheck);
    toolsByName.insert(QLatin1String("ptrcheck"), Parser::Ptrcheck);
    toolsByName.insert(QLatin1String("exp-ptrcheck"), Parser::Ptrcheck);
    toolsByName.insert(QLatin1String("helgrind"), Parser::Helgrind);

    ADD_ENUM(memcheck, ClientCheck)
    ADD_ENUM(memcheck, InvalidFree)
    ADD_ENUM(memcheck, InvalidJump)
    ADD_ENUM(memcheck, InvalidRead)
    ADD_ENUM(memcheck, InvalidWrite)
    ADD_ENUM(memcheck, Leak_DefinitelyLost)
    ADD_ENUM(memcheck, Leak_PossiblyLost)
    ADD_ENUM(memcheck, Leak_StillReachable)
    ADD_ENUM(memcheck, Leak_IndirectlyLost)
    ADD_ENUM(memcheck, MismatchedFree)
    ADD_ENUM(memcheck, Overlap)
    ADD_ENUM(memcheck, SyscallParam)
    ADD_ENUM(memcheck, UninitCondition)
    ADD_ENUM(memcheck, UninitValue)

    ADD_ENUM(helgrind, Race)
    ADD_ENUM(helgrind, UnlockUnlocked)
    ADD_ENUM(helgrind, UnlockForeign)
    ADD_ENUM(helgrind, UnlockBogus)
    ADD_ENUM(helgrind, PthAPIerror)
    ADD_ENUM(helgrind, LockOrder)
    ADD_ENUM(helgrind, Misc)

    ADD_ENUM(ptrcheck, SorG)
    ADD_ENUM(ptrcheck, Heap)
    ADD_ENUM(ptrcheck, Arith)
    ADD_ENUM(ptrcheck, SysParam)
}

#undef ADD_ENUM

static quint64 parseHex(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toULongLong(&ok, 16);
    if (!ok)
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Could not parse hex number from \"%1\" (%2)").arg(str, context));
    return v;
}

static qint64 parseInt64(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toLongLong(&ok);
    if (!ok)
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Could not parse hex number from \"%1\" (%2)").arg(str, context));
    return v;
}

QXmlStreamReader::TokenType Parser::Private::blockingReadNext()
{
    QXmlStreamReader::TokenType token = QXmlStreamReader::Invalid;

    forever {
        token = reader.readNext();

        if (reader.error() == QXmlStreamReader::PrematureEndOfDocumentError) {
            if (reader.device()->waitForReadyRead(1000)) {
                // let's try again
                continue;
            } else {
                // device error, e.g. remote host closed connection, or timeout
                // ### we have no way to know if waitForReadyRead() timed out or failed with a real
                //     error, and sensible heuristics based on QIODevice fail.
                //     - error strings are translated and in any case not guaranteed to stay the same,
                //       so we can't use them.
                //     - errorString().isEmpty() does not work because errorString() is
                //       "network timeout error" if the waitFor... timed out.
                //     - isSequential() [for socket] && isOpen() doesn't work because isOpen()
                //       returns true if the remote host closed the connection.
                // ...so we fall back to knowing it might be a QAbstractSocket.

                QIODevice *dev = reader.device();
                QAbstractSocket *sock = qobject_cast<QAbstractSocket *>(dev);

                if (!sock || sock->state() != QAbstractSocket::ConnectedState) {
                    throw ParserException(dev->errorString());
                }
            }
        } else if (reader.hasError()) {
            throw ParserException(reader.errorString()); //TODO add line, column?
            break;
        } else {
            // read a valid next token
            break;
        }
    }

    return token;
}

bool Parser::Private::notAtEnd() const
{
    return !reader.atEnd()
           || reader.error() == QXmlStreamReader::PrematureEndOfDocumentError;
}

QString Parser::Private::blockingReadElementText()
{
    //analogous to QXmlStreamReader::readElementText(), but blocking. readElementText() doesn't recover from PrematureEndOfData,
    //but always returns a null string if isStartElement() is false (which is the case if it already parts of the text)
    //affects at least Qt <= 4.7.1. Reported as QTBUG-14661.

    if (!reader.isStartElement())
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "trying to read element text although current position is not start of element"));

    QString result;

    forever {
        const QXmlStreamReader::TokenType type = blockingReadNext();
        switch (type) {
        case QXmlStreamReader::Characters:
        case QXmlStreamReader::EntityReference:
            result += reader.text();
            break;
        case QXmlStreamReader::EndElement:
            return result;
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::StartElement:
            throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                              "Unexpected child element while reading element text"));
        default:
            //TODO handle
            throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                              "Unexpected token type %1").arg(type));
            break;
        }
    }
    return QString();
}

void Parser::Private::checkProtocolVersion(const QString &versionStr)
{
    bool ok;
    const int version = versionStr.toInt(&ok);
    if (!ok)
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Could not parse protocol version from \"%1\"").arg(versionStr));
    if (version != 4)
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "XmlProtocol version %1 not supported (supported version: 4)").arg(version));
}

void Parser::Private::checkTool(const QString &reportedStr)
{
    const QHash<QString,Parser::Tool>::ConstIterator reported = toolsByName.find(reportedStr);

    if (reported == toolsByName.constEnd())
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Valgrind tool \"%1\" not supported").arg(reportedStr));

    tool = reported.value();
}

XWhat Parser::Private::parseXWhat()
{
    XWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.name() == QLatin1String("text"))
            what.text = blockingReadElementText();
        else if (reader.name() == QLatin1String("leakedbytes"))
            what.leakedbytes = parseInt64(blockingReadElementText(), QLatin1String("error/xwhat[memcheck]/leakedbytes"));
        else if (reader.name() == QLatin1String("leakedblocks"))
            what.leakedblocks = parseInt64(blockingReadElementText(), QLatin1String("error/xwhat[memcheck]/leakedblocks"));
        else if (reader.name() == QLatin1String("hthreadid"))
            what.hthreadid = parseInt64(blockingReadElementText(), QLatin1String("error/xwhat[memcheck]/hthreadid"));
        else if (reader.isStartElement())
            reader.skipCurrentElement();
    }
    return what;
}

XauxWhat Parser::Private::parseXauxWhat()
{
    XauxWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.name() == QLatin1String("text"))
            what.text = blockingReadElementText();
        else if (reader.name() == QLatin1String("file"))
            what.file = blockingReadElementText();
        else if (reader.name() == QLatin1String("dir"))
            what.dir = blockingReadElementText();
        else if (reader.name() == QLatin1String("line"))
            what.line = parseInt64(blockingReadElementText(), QLatin1String("error/xauxwhat/line"));
        else if (reader.name() == QLatin1String("hthreadid"))
            what.hthreadid = parseInt64(blockingReadElementText(), QLatin1String("error/xauxwhat/hthreadid"));
        else if (reader.isStartElement())
            reader.skipCurrentElement();
    }
    return what;
}



MemcheckErrorKind Parser::Private::parseMemcheckErrorKind(const QString &kind)
{
    const QHash<QString,MemcheckErrorKind>::ConstIterator it = errorKindsByName_memcheck.find(kind);
    if (it != errorKindsByName_memcheck.constEnd())
        return *it;
    else
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Unknown memcheck error kind \"%1\"").arg(kind));
}

HelgrindErrorKind Parser::Private::parseHelgrindErrorKind(const QString &kind)
{
    const QHash<QString,HelgrindErrorKind>::ConstIterator it = errorKindsByName_helgrind.find(kind);
    if (it != errorKindsByName_helgrind.constEnd())
        return *it;
    else
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Unknown helgrind error kind \"%1\"").arg(kind));
}

PtrcheckErrorKind Parser::Private::parsePtrcheckErrorKind(const QString &kind)
{
    const QHash<QString,PtrcheckErrorKind>::ConstIterator it = errorKindsByName_ptrcheck.find(kind);
    if (it != errorKindsByName_ptrcheck.constEnd())
        return *it;
    else
        throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                          "Unknown ptrcheck error kind \"%1\"").arg(kind));
}

int Parser::Private::parseErrorKind(const QString &kind)
{
    switch (tool) {
    case Memcheck:
        return parseMemcheckErrorKind(kind);
    case Ptrcheck:
        return parsePtrcheckErrorKind(kind);
    case Helgrind:
        return parseHelgrindErrorKind(kind);
    case Unknown:
    default:
        break;
    }
    throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                      "Could not parse error kind, tool not yet set."));
}

static Status::State parseState(const QString &state)
{
    if (state == QLatin1String("RUNNING"))
        return Status::Running;
    if (state == QLatin1String("FINISHED"))
        return Status::Finished;
    throw ParserException(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                      "Unknown state \"%1\"").arg(state));
}

void Parser::Private::reportInternalError(const QString &e)
{
    errorString = e;
    emit q->internalError(e);
}

static Stack makeStack( const XauxWhat &xauxwhat, const QVector<Frame> &frames)
{
    Stack s;
    s.setFrames(frames);
    s.setFile(xauxwhat.file);
    s.setDirectory(xauxwhat.dir);
    s.setLine(xauxwhat.line);
    s.setHelgrindThreadId(xauxwhat.hthreadid);
    s.setAuxWhat(xauxwhat.text);
    return s;
}

void Parser::Private::parse_error()
{
    Error e;
    QVector<QVector<Frame> > frames;
    XauxWhat currentAux;
    QVector<XauxWhat> auxs;

    int lastAuxWhat = -1;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement())
            lastAuxWhat++;
        if (reader.name() == QLatin1String("unique"))
            e.setUnique(parseHex(blockingReadElementText(), QLatin1String("unique")));
        else if ( reader.name() == QLatin1String("tid"))
            e.setTid(parseInt64(blockingReadElementText(), QLatin1String("error/tid")));
        else if (reader.name() == QLatin1String("kind")) //TODO this is memcheck-specific:
            e.setKind(parseErrorKind(blockingReadElementText()));
        else if (reader.name() == QLatin1String("suppression"))
            e.setSuppression(parse_suppression());
        else if (reader.name() == QLatin1String("xwhat")) {
            const XWhat xw = parseXWhat();
            e.setWhat(xw.text);
            e.setLeakedBlocks(xw.leakedblocks);
            e.setLeakedBytes(xw.leakedbytes);
            e.setHelgrindThreadId(xw.hthreadid);
        }
        else if (reader.name() == QLatin1String("what"))
            e.setWhat(blockingReadElementText());
        else if (reader.name() == QLatin1String("xauxwhat")) {
            if (!currentAux.text.isEmpty())
                auxs.push_back(currentAux);
            currentAux = parseXauxWhat();
        }
        else if (reader.name() == QLatin1String("auxwhat")) {
            const QString aux = blockingReadElementText();
            //concatenate multiple consecutive <auxwhat> tags
            if (lastAuxWhat > 1) {
                if (!currentAux.text.isEmpty())
                    auxs.push_back(currentAux);
                currentAux.clear();
                currentAux.text = aux;
            } else {
                if (!currentAux.text.isEmpty())
                    currentAux.text.append(QLatin1Char(' '));
                currentAux.text.append(aux);
            }
            lastAuxWhat = 0;
        }
        else if (reader.name() == QLatin1String("stack")) {
            frames.push_back(parse_stack());
        }
        else if (reader.isStartElement())
            reader.skipCurrentElement();
    }

    if (!currentAux.text.isEmpty())
        auxs.push_back(currentAux);

    //if we have less xaux/auxwhats than stacks, prepend empty xauxwhats
    //(the first frame usually has not xauxwhat in helgrind and memcheck)
    while (auxs.size() < frames.size())
        auxs.prepend(XauxWhat());

    QVector<Stack> stacks;
    for (int i = 0; i < auxs.size(); ++i)
        stacks.append(makeStack(auxs[i], frames[i]));
    e.setStacks(stacks);

    emit q->error(e);
}

Frame Parser::Private::parse_frame()
{
    Frame frame;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("ip"))
                frame.setInstructionPointer(parseHex(blockingReadElementText(), QLatin1String("error/frame/ip")));
            else if (reader.name() == QLatin1String("obj"))
                frame.setObject(blockingReadElementText());
            else if (reader.name() == QLatin1String("fn"))
                frame.setFunctionName( blockingReadElementText());
            else if (reader.name() == QLatin1String("dir"))
                frame.setDirectory(blockingReadElementText());
            else if (reader.name() == QLatin1String("file"))
                frame.setFile( blockingReadElementText());
            else if (reader.name() == QLatin1String("line"))
                frame.setLine(parseInt64(blockingReadElementText(), QLatin1String("error/frame/line")));
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    return frame;
}

void Parser::Private::parse_announcethread()
{
    AnnounceThread at;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("hthreadid"))
                at.setHelgrindThreadId(parseInt64(blockingReadElementText(), QLatin1String("announcethread/hthreadid")));
            else if (reader.name() == QLatin1String("stack"))
                at.setStack(parse_stack());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    emit q->announceThread(at);
}

void Parser::Private::parse_errorcounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("pair")) {
                qint64 unique = 0;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (reader.isEndElement())
                        break;
                    if (reader.isStartElement()) {
                        if (reader.name() == QLatin1String("unique"))
                            unique = parseHex(blockingReadElementText(), QLatin1String("errorcounts/pair/unique"));
                        else if (reader.name() == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), QLatin1String("errorcounts/pair/count"));
                        else if (reader.isStartElement())
                            reader.skipCurrentElement();
                    }
                }
                emit q->errorCount(unique, count);
            }
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }
}


void Parser::Private::parse_suppcounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("pair")) {
                QString name;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (reader.isEndElement())
                        break;
                    if (reader.isStartElement()) {
                        if (reader.name() == QLatin1String("name"))
                            name = blockingReadElementText();
                        else if (reader.name() == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), QLatin1String("suppcounts/pair/count"));
                        else if (reader.isStartElement())
                            reader.skipCurrentElement();
                    }
                }
                emit q->suppressionCount(name, count);
            }
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }
}

void Parser::Private::parse_status()
{
    Status s;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("state"))
                s.setState(parseState(blockingReadElementText()));
            else if (reader.name() == QLatin1String("time"))
                s.setTime(blockingReadElementText());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    emit q->status(s);
}

QVector<Frame> Parser::Private::parse_stack()
{
    QVector<Frame> frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("frame"))
                frames.append(parse_frame());
        }
    }

    return frames;
}

SuppressionFrame Parser::Private::parse_suppFrame()
{
    SuppressionFrame frame;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("obj"))
                frame.setObject(blockingReadElementText());
            else if (reader.name() == QLatin1String("fun"))
                frame.setFunction( blockingReadElementText());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    return frame;
}

Suppression Parser::Private::parse_suppression()
{
    Suppression supp;
    QVector<SuppressionFrame> frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("sname"))
                supp.setName(blockingReadElementText());
            else if (reader.name() == QLatin1String("skind"))
                supp.setKind(blockingReadElementText());
            else if (reader.name() == QLatin1String("skaux"))
                supp.setAuxKind(blockingReadElementText());
            else if (reader.name() == QLatin1String("rawtext"))
                supp.setRawText(blockingReadElementText());
            else if (reader.name() == QLatin1String("sframe"))
                frames.push_back(parse_suppFrame());
        }
    }

    supp.setFrames(frames);
    return supp;
}

void Parser::Private::parse(QIODevice *device)
{
    QTC_ASSERT(device, return);
    reader.setDevice(device);

    try {
        while (notAtEnd()) {
            blockingReadNext();
            if (reader.name() == QLatin1String("error"))
                parse_error();
            else if (reader.name() == QLatin1String("announcethread"))
                parse_announcethread();
            else if (reader.name() == QLatin1String("status"))
                parse_status();
            else if (reader.name() == QLatin1String("errorcounts"))
                parse_errorcounts();
            else if (reader.name() == QLatin1String("suppcounts"))
                parse_suppcounts();
            else if (reader.name() == QLatin1String("protocolversion"))
                checkProtocolVersion(blockingReadElementText());
            else if (reader.name() == QLatin1String("protocoltool"))
                checkTool(blockingReadElementText());
        }
    } catch (const ParserException &e) {
        reportInternalError(e.message());
    } catch (...) {
        reportInternalError(QCoreApplication::translate("Valgrind::XmlProtocol::Parser",
                                                        "Unexpected exception caught during parsing."));
    }
    emit q->finished();
}

Parser::Parser(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

Parser::~Parser()
{
    delete d;
}

QString Parser::errorString() const
{
    return d->errorString;
}

void Parser::parse(QIODevice *device)
{
    d->parse(device);
}
