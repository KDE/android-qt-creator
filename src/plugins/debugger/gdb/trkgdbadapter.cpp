/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "trkgdbadapter.h"

#include "gdbmi.h"
#include "launcher.h"
#include "symbiandevicemanager.h"
#include "s60debuggerbluetoothstarter.h"
#include "bluetoothlistener_gui.h"

#include "registerhandler.h"
#include "threadshandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "watchutils.h"
#ifndef STANDALONE_RUNNER
#include "gdbengine.h"
#endif

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/savedaction.h>

#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#ifdef Q_OS_WIN
#  include "dbgwinutils.h"
#else
#  include <sys/types.h>
#  include <unistd.h>
#endif

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&TrkGdbAdapter::callback), \
    STRINGIFY(callback)

#define TrkCB(s) TrkCallback(this, &TrkGdbAdapter::s)

using namespace trk;

namespace Debugger {
namespace Internal {
using namespace Symbian;

static inline void appendByte(QByteArray *ba, trk::byte b) { ba->append(b); }

///////////////////////////////////////////////////////////////////////////
//
// TrkGdbAdapter
//
///////////////////////////////////////////////////////////////////////////

/* Thread handling:
 * TRK does not report thread creation/termination. So, if we receive
 * a stop in a different thread, we store an additional thread in snapshot.
 * When continuing in trkContinueAll(), we delete this thread, since we cannot
 * know whether it will exist at the next stop.
 * Also note that threads continue running in Symbian even if one crashes.
 * TODO: Stop all threads once one stops? */

TrkGdbAdapter::TrkGdbAdapter(GdbEngine *engine) :
    AbstractGdbAdapter(engine),
    m_running(false),
    m_gdbAckMode(true),
    m_verbose(0)
{
    m_bufferedMemoryRead = true;
    // Disable buffering if gdb's dcache is used.
    m_bufferedMemoryRead = false;

    m_gdbServer = 0;
    m_gdbConnection = 0;
    m_snapshot.reset();
#ifdef Q_OS_WIN
    const unsigned long portOffset = winGetCurrentProcessId() % 100;
#else
    const uid_t portOffset = getuid();
#endif
    m_gdbServerName = _("127.0.0.1:%1").arg(2222 + portOffset);

    setVerbose(debuggerCore()->boolSetting(VerboseLog));

    connect(debuggerCore()->action(VerboseLog), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setVerbose(QVariant)));
}

TrkGdbAdapter::~TrkGdbAdapter()
{
    cleanup();
    logMessage("Shutting down.\n");
}

void TrkGdbAdapter::setVerbose(const QVariant &value)
{
    setVerbose(value.toInt());
}

void TrkGdbAdapter::setVerbose(int verbose)
{
    m_verbose = verbose;
    if (!m_trkDevice.isNull())
        m_trkDevice->setVerbose(m_verbose);
}

void TrkGdbAdapter::trkLogMessage(const QString &msg)
{
    logMessage("TRK " + msg);
}

void TrkGdbAdapter::setGdbServerName(const QString &name)
{
    m_gdbServerName = name;
}

QString TrkGdbAdapter::gdbServerIP() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return m_gdbServerName;
    return m_gdbServerName.left(pos);
}

uint TrkGdbAdapter::gdbServerPort() const
{
    int pos = m_gdbServerName.indexOf(':');
    if (pos == -1)
        return 0;
    return m_gdbServerName.mid(pos + 1).toUInt();
}

QByteArray TrkGdbAdapter::trkContinueMessage(uint threadId)
{
    QByteArray ba;
    appendInt(&ba, m_session.pid);
    appendInt(&ba, threadId);
    return ba;
}

QByteArray TrkGdbAdapter::trkWriteRegisterMessage(trk::byte reg, uint value)
{
    QByteArray ba;
    appendByte(&ba, 0); // ?
    appendShort(&ba, reg);
    appendShort(&ba, reg);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    appendInt(&ba, value);
    return ba;
}

QByteArray TrkGdbAdapter::trkReadMemoryMessage(const MemoryRange &range)
{
    return trk::Launcher::readMemoryMessage(m_session.pid, m_session.tid, range.from, range.size());
}

QByteArray TrkGdbAdapter::trkWriteMemoryMessage(uint addr, const QByteArray &data)
{
    QByteArray ba;
    ba.reserve(11 + data.size());
    appendByte(&ba, 0x08); // Options, FIXME: why?
    appendShort(&ba, data.size());
    appendInt(&ba, addr);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    ba.append(data);
    return ba;
}

QByteArray TrkGdbAdapter::trkStepRangeMessage()
{
    //qDebug() << "STEP ON " << hexxNumber(m_snapshot.registers[RegisterPC]);
    uint from = m_snapshot.lineFromAddress;
    uint to = m_snapshot.lineToAddress;
    const uint pc = m_snapshot.registerValue(m_session.tid, RegisterPC);
    trk::byte option = 0x01; // Step into.
    if (m_snapshot.stepOver)
        option = 0x11;  // Step over.
    if (from <= pc && pc <= to) {
        //to = qMax(to - 4, from);
        //to = qMax(to - 4, from);
        showMessage("STEP IN " + hexxNumber(from) + " " + hexxNumber(to)
            + " INSTEAD OF " + hexxNumber(pc));
    } else {
        from = pc;
        to = pc;
    }
    logMessage(QString::fromLatin1("Stepping from 0x%1 to 0x%2 (current PC=0x%3), option 0x%4").
               arg(from, 0, 16).arg(to, 0, 16).arg(pc).arg(option, 0, 16));
    QByteArray ba;
    ba.reserve(17);
    appendByte(&ba, option);
    appendInt(&ba, from); // Start address
    appendInt(&ba, to); // End address
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    return ba;
}

QByteArray TrkGdbAdapter::trkDeleteProcessMessage()
{
    QByteArray ba;
    ba.reserve(6);
    appendByte(&ba, 0); // ?
    appendByte(&ba, 0); // Sub-command: Delete Process
    appendInt(&ba, m_session.pid);
    return ba;
}

QByteArray TrkGdbAdapter::trkInterruptMessage()
{
    QByteArray ba;
    ba.reserve(9);
    // Stop the thread (2) or the process (1) or the whole system (0).
    // We choose 2, as 1 does not seem to work.
    appendByte(&ba, 2);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.mainTid); // threadID: 4 bytes Variable number of bytes.
    return ba;
}

void TrkGdbAdapter::emitDelayedInferiorSetupFailed(const QString &msg)
{
    m_adapterFailMessage = msg;
    QTimer::singleShot(0, this, SLOT(slotEmitDelayedInferiorSetupFailed()));
}

void TrkGdbAdapter::slotEmitDelayedInferiorSetupFailed()
{
    m_engine->notifyInferiorSetupFailed(m_adapterFailMessage);
}


void TrkGdbAdapter::logMessage(const QString &msg, int logChannel)
{
    if (m_verbose || logChannel != LogDebug)
        showMessage("TRK LOG: " + msg, logChannel);
    MEMORY_DEBUG("GDB: " << msg);
}

//
// Gdb
//
void TrkGdbAdapter::handleGdbConnection()
{
    logMessage("HANDLING GDB CONNECTION");
    QTC_ASSERT(m_gdbConnection == 0, /**/);
    m_gdbConnection = m_gdbServer->nextPendingConnection();
    QTC_ASSERT(m_gdbConnection, return);
    connect(m_gdbConnection, SIGNAL(disconnected()),
            m_gdbConnection, SLOT(deleteLater()));
    connect(m_gdbConnection, SIGNAL(readyRead()),
            this, SLOT(readGdbServerCommand()));
}

static inline QString msgGdbPacket(const QString &p)
{
    return QLatin1String("gdb:                              ") + p;
}

void TrkGdbAdapter::readGdbServerCommand()
{
    QTC_ASSERT(m_gdbConnection, return);
    QByteArray packet = m_gdbConnection->readAll();
    m_gdbReadBuffer.append(packet);

    logMessage("gdb: -> " + currentTime() + ' ' + QString::fromAscii(packet));
    if (packet != m_gdbReadBuffer)
        logMessage("buffer: " + m_gdbReadBuffer);

    QByteArray &ba = m_gdbReadBuffer;
    while (ba.size()) {
        char code = ba.at(0);
        ba = ba.mid(1);

        if (code == '+') {
            //logMessage("ACK");
            continue;
        }

        if (code == '-') {
            logMessage("NAK: Retransmission requested", LogError);
            // This seems too harsh.
            //emit adapterCrashed("Communication problem encountered.");
            continue;
        }

        if (code == char(0x03)) {
            logMessage("INTERRUPT RECEIVED");
            interruptInferior();
            continue;
        }

        if (code != '$') {
            logMessage("Broken package (2) " + quoteUnprintableLatin1(ba)
                + hexNumber(code), LogError);
            continue;
        }

        int pos = ba.indexOf('#');
        if (pos == -1) {
            logMessage("Invalid checksum format in "
                + quoteUnprintableLatin1(ba), LogError);
            continue;
        }

        bool ok = false;
        uint checkSum = ba.mid(pos + 1, 2).toUInt(&ok, 16);
        if (!ok) {
            logMessage("Invalid checksum format 2 in "
                + quoteUnprintableLatin1(ba), LogError);
            return;
        }

        //logMessage(QString("Packet checksum: %1").arg(checkSum));
        trk::byte sum = 0;
        for (int i = 0; i < pos; ++i)
            sum += ba.at(i);

        if (sum != checkSum) {
            logMessage(QString("ERROR: Packet checksum wrong: %1 %2 in "
                + quoteUnprintableLatin1(ba)).arg(checkSum).arg(sum), LogError);
        }

        QByteArray cmd = ba.left(pos);
        ba.remove(0, pos + 3);
        handleGdbServerCommand(cmd);
    }
}

bool TrkGdbAdapter::sendGdbServerPacket(const QByteArray &packet, bool doFlush)
{
    if (!m_gdbConnection) {
        logMessage(_("Cannot write to gdb: No connection (%1)")
            .arg(_(packet)), LogError);
        return false;
    }
    if (m_gdbConnection->state() != QAbstractSocket::ConnectedState) {
        logMessage(_("Cannot write to gdb: Not connected (%1)")
            .arg(_(packet)), LogError);
        return false;
    }
    if (m_gdbConnection->write(packet) == -1) {
        logMessage(_("Cannot write to gdb: %1 (%2)")
            .arg(m_gdbConnection->errorString()).arg(_(packet)), LogError);
        return false;
    }
    if (doFlush)
        m_gdbConnection->flush();
    return true;
}

void TrkGdbAdapter::sendGdbServerAck()
{
    if (!m_gdbAckMode)
        return;
    logMessage("gdb: <- +");
    sendGdbServerPacket(QByteArray(1, '+'), false);
}

void TrkGdbAdapter::sendGdbServerMessage(const QByteArray &msg, const QByteArray &logNote)
{
    trk::byte sum = 0;
    for (int i = 0; i != msg.size(); ++i)
        sum += msg.at(i);

    char checkSum[30];
    qsnprintf(checkSum, sizeof(checkSum) - 1, "%02x ", sum);

    //logMessage(QString("Packet checksum: %1").arg(sum));

    QByteArray packet;
    packet.append('$');
    packet.append(msg);
    packet.append('#');
    packet.append(checkSum);
    int pad = qMax(0, 24 - packet.size());
    logMessage("gdb: <- " + currentTime() + ' ' + packet + QByteArray(pad, ' ') + logNote);
    sendGdbServerPacket(packet, true);
}

void TrkGdbAdapter::sendGdbServerMessageAfterTrkResponse(const QByteArray &msg,
    const QByteArray &logNote)
{
    QByteArray ba = msg + char(1) + logNote;
    sendTrkMessage(TRK_WRITE_QUEUE_NOOP_CODE, TrkCB(reportToGdb), "", ba); // Answer gdb
}

void TrkGdbAdapter::reportToGdb(const TrkResult &result)
{
    QByteArray message = result.cookie.toByteArray();
    QByteArray note;
    int pos = message.lastIndexOf(char(1)); // HACK
    if (pos != -1) {
        note = message.mid(pos + 1);
        message = message.left(pos);
    }
    message.replace("@CODESEG@", hexNumber(m_session.codeseg));
    message.replace("@DATASEG@", hexNumber(m_session.dataseg));
    message.replace("@PID@", hexNumber(m_session.pid));
    message.replace("@TID@", hexNumber(m_session.tid));
    sendGdbServerMessage(message, note);
}

QByteArray TrkGdbAdapter::trkBreakpointMessage(uint addr, uint len, bool armMode)
{
    QByteArray ba;
    appendByte(&ba, 0x82);  // unused option
    appendByte(&ba, armMode /*bp.mode == ArmMode*/ ? 0x00 : 0x01);
    appendInt(&ba, addr);
    appendInt(&ba, len);
    appendInt(&ba, 0x00000001);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, 0xFFFFFFFF);
    return ba;
}

static QByteArray msgStepRangeReceived(unsigned from, unsigned to, bool over)
{
    QByteArray rc = "Stepping range received for step ";
    rc += over ? "over" : "into";
    rc += " (0x";
    rc += QByteArray::number(from, 16);
    rc += " to 0x";
    rc += QByteArray::number(to, 16);
    rc += ')';
    return rc;
}

void TrkGdbAdapter::handleGdbServerCommand(const QByteArray &cmd)
{
    // http://sourceware.org/gdb/current/onlinedocs/gdb_34.html
    if (0) {}

    else if (cmd == "!") {
        sendGdbServerAck();
        //sendGdbServerMessage("", "extended mode not enabled");
        sendGdbServerMessage("OK", "extended mode enabled");
    }

    else if (cmd.startsWith('?')) {
        logMessage(msgGdbPacket(QLatin1String("Query halted")));
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        sendGdbServerAck();
        // The command below will trigger fetching a stack trace while
        // the process does not seem to be fully functional. Most notably
        // the PC points to a 0x9..., which is not in "our" range
        //sendGdbServerMessage("T05library:r;", "target halted (library load)");
        //sendGdbServerMessage("S05", "target halted (trap)");
        sendGdbServerMessage("S00", "target halted (trap)");
        //sendGdbServerMessage("O" + QByteArray("Starting...").toHex());
    }

    else if (cmd == "c") {
        logMessage(msgGdbPacket(QLatin1String("Continue")));
        sendGdbServerAck();
        m_running = true;
        trkContinueAll("gdb 'c'");
    }

    else if (cmd.startsWith('C')) {
        logMessage(msgGdbPacket(QLatin1String("Continue with signal")));
        // C sig[;addr] Continue with signal sig (hex signal number)
        //Reply: See section D.3 Stop Reply Packets, for the reply specifications.
        //TODO: Meaning of the message is not clear.
        sendGdbServerAck();
        bool ok = false;
        const uint signalNumber = cmd.mid(1).toUInt(&ok, 16);
        logMessage(QString::fromLatin1("Not implemented 'Continue with signal' %1: ").arg(signalNumber), LogWarning);
        sendGdbServerMessage("O" + QByteArray("Console output").toHex());
        sendGdbServerMessage("W81"); // "Process exited with result 1
        trkContinueAll("gdb 'C'");
    }

    else if (cmd.startsWith('D')) {
        sendGdbServerAck();
        sendGdbServerMessage("OK", "shutting down");
    }

    else if (cmd == "g") {
        // Read general registers.
        if (m_snapshot.registersValid(m_session.tid)) {
            //qDebug() << "Using cached register contents";
            logMessage(msgGdbPacket(QLatin1String("Read registers")));
            sendGdbServerAck();
            reportRegisters();
        } else {
            //qDebug() << "Fetching register contents";
            sendGdbServerAck();
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegisters),
                Launcher::readRegistersMessage(m_session.pid, m_session.tid));
        }
    }

    else if (cmd == "gg") {
        // Force re-reading general registers for debugging purpose.
        sendGdbServerAck();
        m_snapshot.setRegistersValid(m_session.tid, false);
        sendTrkMessage(0x12,
            TrkCB(handleAndReportReadRegisters),
            Launcher::readRegistersMessage(m_session.pid, m_session.tid));
    }

    else if (cmd.startsWith("salstep,")) {
        // Receive address range for current line for future use when stepping.
        sendGdbServerAck();
        m_snapshot.parseGdbStepRange(cmd, false);
        sendGdbServerMessage("", msgStepRangeReceived(m_snapshot.lineFromAddress, m_snapshot.lineToAddress, m_snapshot.stepOver));
    }

    else if (cmd.startsWith("salnext,")) {
        // Receive address range for current line for future use when stepping.
        sendGdbServerAck();
        m_snapshot.parseGdbStepRange(cmd, true);
        sendGdbServerMessage("", msgStepRangeReceived(m_snapshot.lineFromAddress, m_snapshot.lineToAddress, m_snapshot.stepOver));
    }

    else if (cmd.startsWith("Hc")) {
        sendGdbServerAck();
        gdbSetCurrentThread(cmd, "Set current thread for step & continue ");
    }

    else if (cmd.startsWith("Hg")) {
        sendGdbServerAck();
        gdbSetCurrentThread(cmd, "Set current thread ");
    }

    else if (cmd == "k" || cmd.startsWith("vKill")) {
        trkKill();
    }

    else if (cmd.startsWith('m')) {
        logMessage(msgGdbPacket(QLatin1String("Read memory")));
        // m addr,length
        sendGdbServerAck();
        const QPair<quint64, unsigned> addrLength = parseGdbReadMemoryRequest(cmd);
        if (addrLength.second) {
            readMemory(addrLength.first, addrLength.second, m_bufferedMemoryRead);
        } else {
            sendGdbServerMessage("E20", "Error " + cmd);
        }
    }

    else if (cmd.startsWith('p')) {
        logMessage(msgGdbPacket(QLatin1String("read register")));
        // 0xf == current instruction pointer?
        //sendGdbServerMessage("0000", "current IP");
        sendGdbServerAck();
        bool ok = false;
        const uint registerNumber = cmd.mid(1).toUInt(&ok, 16);
        const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
        QTC_ASSERT(threadIndex != -1, return)
        const Symbian::Thread &thread =  m_snapshot.threadInfo[threadIndex];
        if (thread.registerValid) {
            sendGdbServerMessage(thread.gdbReportSingleRegister(registerNumber), thread.gdbSingleRegisterLogMessage(registerNumber));
        } else {
            //qDebug() << "Fetching single register";
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegister),
                Launcher::readRegistersMessage(m_session.pid, m_session.tid), registerNumber);
        }
    }

    else if (cmd.startsWith('P')) {
        logMessage(msgGdbPacket(QLatin1String("write register")));
        // $Pe=70f96678#d3
        sendGdbServerAck();
        const QPair<uint, uint> regnumValue = parseGdbWriteRegisterWriteRequest(cmd);
        // FIXME: Assume all goes well.
        m_snapshot.setRegisterValue(m_session.tid, regnumValue.first, regnumValue.second);
        QByteArray ba = trkWriteRegisterMessage(regnumValue.first, regnumValue.second);
        sendTrkMessage(0x13, TrkCB(handleWriteRegister), ba, "Write register");
        // Note that App TRK refuses to write registers 13 and 14
    }

    else if (cmd == "qAttached") {
        //$qAttached#8f
        // 1: attached to an existing process
        // 0: created a new process
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, '0'), "new process created");
        //sendGdbServerMessage('1', "attached to existing process");
        //sendGdbServerMessage("E01", "new process created");
    }

    else if (cmd.startsWith("qC")) {
        logMessage(msgGdbPacket(QLatin1String("query thread id")));
        // Return the current thread ID
        //$qC#b4
        sendGdbServerAck();
        sendGdbServerMessageAfterTrkResponse("QC@TID@");
    }

    else if (cmd.startsWith("qSupported")) {
        //$qSupported#37
        //$qSupported:multiprocess+#c6
        //logMessage("Handling 'qSupported'");
        sendGdbServerAck();
        sendGdbServerMessage(Symbian::gdbQSupported);
    }

    // Tracepoint handling as of gdb 7.2 onwards
    else if (cmd == "qTStatus") { // Tracepoints
        sendGdbServerAck();
        sendGdbServerMessage("T0;tnotrun:0", QByteArray("No trace experiment running"));
    }
    // Trace variables  as of gdb 7.2 onwards
    else if (cmd == "qTfV" || cmd == "qTsP" || cmd == "qTfP") {
        sendGdbServerAck();
        sendGdbServerMessage("l", QByteArray("No trace points"));
    }

    else if (cmd.startsWith("qThreadExtraInfo")) {
        // $qThreadExtraInfo,1f9#55
        sendGdbServerAck();
        sendGdbServerMessage(m_snapshot.gdbQThreadExtraInfo(cmd));
    }

    else if (cmd == "qfDllInfo") {
        // That's the _first_ query package.
        // Happens with  gdb 6.4.50.20060226-cvs / CodeSourcery.
        // Never made it into FSF gdb that got qXfer:libraries:read instead.
        // http://sourceware.org/ml/gdb/2007-05/msg00038.html
        // Name=hexname,TextSeg=textaddr[,DataSeg=dataaddr]
        sendGdbServerAck();
        sendGdbServerMessage(m_session.gdbQsDllInfo(), "library information transferred");
    }

    else if (cmd == "qsDllInfo") {
        // That's a following query package
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, 'l'), "library information transfer finished");
    }

    else if (cmd == "qPacketInfo") {
        // happens with  gdb 6.4.50.20060226-cvs / CodeSourcery
        // deprecated by qSupported?
        sendGdbServerAck();
        sendGdbServerMessage("", "FIXME: nothing?");
    }

    else if (cmd == "qOffsets") {
        sendGdbServerAck();
        sendGdbServerMessageAfterTrkResponse("TextSeg=@CODESEG@;DataSeg=@DATASEG@");
    }

    else if (cmd == "qSymbol::") {
        if (m_verbose)
            logMessage(msgGdbPacket(QLatin1String("notify can handle symbol lookup")));
        // Notify the target that GDB is prepared to serve symbol lookup requests.
        sendGdbServerAck();
        if (1)
            sendGdbServerMessage("OK", "no further symbols needed");
        else
            sendGdbServerMessage("qSymbol:" + QByteArray("_Z7E32Mainv").toHex(),
                "ask for more");
    }

    else if (cmd.startsWith("qXfer:features:read:target.xml:")) {
        //  $qXfer:features:read:target.xml:0,7ca#46...Ack
        sendGdbServerAck();
        sendGdbServerMessage(Symbian::gdbArchitectureXml);
    }

    else if (cmd == "qfThreadInfo") {
        // That's the _first_ query package.
        sendGdbServerAck();
        sendGdbServerMessage(m_snapshot.gdbQsThreadInfo(), "thread information transferred");
    }

    else if (cmd == "qsThreadInfo") {
        // That's a following query package
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, 'l'), "thread information transfer finished");
    }

    else if (cmd.startsWith("qXfer:libraries:read")) {
        sendGdbServerAck();
        sendGdbServerMessage(m_session.gdbLibraryList(), "library information transferred");
    }

    else if (cmd == "QStartNoAckMode") {
        //$qSupported#37
        logMessage("Handling 'QStartNoAckMode'");
        sendGdbServerAck();
        sendGdbServerMessage("OK", "ack no-ack mode");
        m_gdbAckMode = false;
    }

    else if (cmd.startsWith("QPassSignals")) {
        // list of signals to pass directly to inferior
        // $QPassSignals:e;10;14;17;1a;1b;1c;21;24;25;4c;#8f
        // happens only if "QPassSignals+;" is qSupported
        sendGdbServerAck();
        // FIXME: use the parameters
        sendGdbServerMessage("OK", "passing signals accepted");
    }

    else if (cmd == "s" || cmd.startsWith("vCont;s")) {
        const uint pc = m_snapshot.registerValue(m_session.tid, RegisterPC);
        logMessage(msgGdbPacket(QString::fromLatin1("Step range from 0x%1").
                                arg(pc, 0, 16)));
        sendGdbServerAck();
        //m_snapshot.reset();
        m_running = true;
        QByteArray ba = trkStepRangeMessage();
        sendTrkMessage(0x19, TrkCB(handleStep), ba, "Step range");
    }

    else if (cmd.startsWith('T')) {
        // FIXME: check whether thread is alive
        sendGdbServerAck();
        sendGdbServerMessage("OK"); // pretend all is well
        //sendGdbServerMessage("E nn");
    }

    else if (cmd == "vCont?") {
        // actions supported by the vCont packet
        sendGdbServerAck();
        //sendGdbServerMessage("OK"); // we don't support vCont.
        sendGdbServerMessage("vCont;c;C;s;S");
    }

    else if (cmd == "vCont;c") {
        // vCont[;action[:thread-id]]...'
        sendGdbServerAck();
        //m_snapshot.reset();
        m_running = true;
        trkContinueAll("gdb 'vCont;c'");
    }

    else if (cmd.startsWith("Z0,") || cmd.startsWith("Z1,")) {
        // Insert breakpoint
        sendGdbServerAck();
        logMessage(msgGdbPacket(QLatin1String("Insert breakpoint")));
        // $Z0,786a4ccc,4#99
        const QPair<quint64, unsigned> addrLen = parseGdbSetBreakpointRequest(cmd);
        if (addrLen.first) {
            //qDebug() << "ADDR: " << hexNumber(addr) << " LEN: " << len;
            logMessage(_("Inserting breakpoint at 0x%1, %2")
                .arg(addrLen.first, 0, 16).arg(addrLen.second));
            const bool armMode = addrLen.second == 4;
            const QByteArray ba = trkBreakpointMessage(addrLen.first, addrLen.second, armMode);
            sendTrkMessage(0x1B, TrkCB(handleAndReportSetBreakpoint), ba, QVariant(addrLen.first));
        } else {
            logMessage("MISPARSED BREAKPOINT '" + cmd + "')", LogError);
        }
    }

    else if (cmd.startsWith("z0,") || cmd.startsWith("z1,")) {
        // Remove breakpoint
        sendGdbServerAck();
        logMessage(msgGdbPacket(QLatin1String("Remove breakpoint")));
        // $z0,786a4ccc,4#99
        const int pos = cmd.lastIndexOf(',');
        bool ok = false;
        const uint addr = cmd.mid(3, pos - 3).toUInt(&ok, 16);
        const uint len = cmd.mid(pos + 1).toUInt(&ok, 16);
        const uint bp = m_session.addressToBP[addr];
        if (bp == 0) {
            logMessage(_("NO RECORDED BP AT 0x%1, %2")
                .arg(addr, 0, 16).arg(len), LogError);
            sendGdbServerMessage("E00");
        } else {
            m_session.addressToBP.remove(addr);
            QByteArray ba;
            appendInt(&ba, bp);
            sendTrkMessage(0x1C, TrkCB(handleClearBreakpoint), ba, addr);
        }
    }

    else if (cmd.startsWith("qPart:") || cmd.startsWith("qXfer:"))  {
        QByteArray data = cmd.mid(1 + cmd.indexOf(':'));
        // "qPart:auxv:read::0,147": Read OS auxiliary data (see info aux)
        bool handled = false;
        if (data.startsWith("auxv:read::")) {
            const int offsetPos = data.lastIndexOf(':') + 1;
            const int commaPos = data.lastIndexOf(',');
            if (commaPos != -1) {
                bool ok1 = false, ok2 = false;
                const int offset = data.mid(offsetPos,  commaPos - offsetPos)
                    .toUInt(&ok1, 16);
                const int length = data.mid(commaPos + 1).toUInt(&ok2, 16);
                if (ok1 && ok2) {
                    const QString msg = _("Read of OS auxiliary "
                        "vector (%1, %2) not implemented.").arg(offset).arg(length);
                    logMessage(msgGdbPacket(msg), LogWarning);
                    sendGdbServerMessage("E20", msg.toLatin1());
                    handled = true;
                }
            }
        } // auxv read

        if (!handled) {
            const QString msg = QLatin1String("FIXME unknown 'XFER'-request: ")
                + QString::fromAscii(cmd);
            logMessage(msgGdbPacket(msg), LogWarning);
            sendGdbServerMessage("E20", msg.toLatin1());
        }
    } // qPart/qXfer

    else if (cmd.startsWith("X")) {
        logMessage(msgGdbPacket(QLatin1String("Write memory")));
        // X addr,length
        sendGdbServerAck();
        const QPair<quint64, unsigned> addrLength = parseGdbReadMemoryRequest(cmd);
        int pos = cmd.indexOf(':');
        m_snapshot.resetMemory();
        writeMemory(addrLength.first, cmd.mid(pos + 1, addrLength.second));
    }

    else {
        logMessage(msgGdbPacket(QLatin1String("FIXME unknown: ")
            + QString::fromAscii(cmd)), LogWarning);
    }
}

void TrkGdbAdapter::gdbSetCurrentThread(const QByteArray &cmd, const char *why)
{
    // Thread ID from Hg/Hc commands: '-1': All, '0': arbitrary, else hex thread id.
    const QByteArray id = cmd.mid(2);
    const int threadId = id == "-1" ? -1 : id.toInt(0, 16);
    const QByteArray message = QByteArray(why) + QByteArray::number(threadId);
    logMessage(msgGdbPacket(QString::fromLatin1(message)));
    // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
    // for 'other operations.  0 - any thread
    //$Hg0#df
    m_session.tid = threadId <= 0 ? m_session.mainTid : uint(threadId);
    sendGdbServerMessage("OK", message);
}

void TrkGdbAdapter::trkKill()
{
    // Kill inferior process
    logMessage(msgGdbPacket(QLatin1String("kill")));
    sendTrkMessage(0x41, TrkCB(handleDeleteProcess),
        trkDeleteProcessMessage(), "Delete process");
}

void TrkGdbAdapter::trkContinueAll(const char *why)
{
    if (why)
        logMessage(QString::fromLatin1("Continuing %1 threads (%2)").
                   arg(m_snapshot.threadInfo.size()).arg(QString::fromLatin1(why)));

    // Starting from the last one, continue all threads.
    QTC_ASSERT(!m_snapshot.threadInfo.isEmpty(), return; );
    trkContinueNext(m_snapshot.threadInfo.size() - 1);
}

void TrkGdbAdapter::trkContinueNext(int threadIndex)
{
    const uint threadId = m_snapshot.threadInfo.at(threadIndex).id;
    logMessage(QString::fromLatin1("Continuing thread 0x%1 of %2").
               arg(threadId,0, 16).arg(m_snapshot.threadInfo.size()));
    sendTrkMessage(0x18, TrkCallback(this, &TrkGdbAdapter::handleTrkContinueNext),
                   trkContinueMessage(threadId), QVariant(threadIndex));
}

void TrkGdbAdapter::handleTrkContinueNext(const TrkResult &result)
{
    const int index = result.cookie.toInt();
    if (result.errorCode()) {
        logMessage("Error continuing thread: " + result.errorString(), LogError);
        return;
    }
    // Remove the thread (unless main) if it is continued since we
    // do not get thread creation/deletion events
    QTC_ASSERT(index < m_snapshot.threadInfo.size(), return; );
    if (m_snapshot.threadInfo.at(index).id != m_session.mainTid)
        m_snapshot.threadInfo.remove(index);
    if (index > 0 && m_running) // Stopped in-between
        trkContinueNext(index - 1);
}

void TrkGdbAdapter::sendTrkMessage(trk::byte code, TrkCallback callback,
    const QByteArray &data, const QVariant &cookie)
{
    if (m_verbose >= 2)
        logMessage("trk: -> " + QByteArray::number(code, 16) + "  "
            + stringFromArray(data));
    m_trkDevice->sendTrkMessage(code, callback, data, cookie);
}

void TrkGdbAdapter::sendTrkAck(trk::byte token)
{
    //logMessage(QString("SENDING ACKNOWLEDGEMENT FOR TOKEN %1").arg(int(token)));
    m_trkDevice->sendTrkAck(token);
}

void TrkGdbAdapter::handleTrkError(const QString &msg)
{
    logMessage("## TRK ERROR: " + msg, LogError);
    m_engine->handleAdapterCrashed("TRK problem encountered:\n" + msg);
}

void TrkGdbAdapter::handleTrkResult(const TrkResult &result)
{
    if (m_verbose >= 2)
        logMessage("trk: <- " + result.toString());
    if (result.isDebugOutput) {
        // It looks like those messages _must not_ be acknowledged.
        // If we do so, TRK will complain about wrong sequencing.
        //sendTrkAck(result.token);
        logMessage(QString::fromAscii(result.data), AppOutput);
        sendGdbServerMessage("O" + result.data.toHex());
        return;
    }
    //logMessage("READ TRK " + result.toString());
    QByteArray prefix = "READ BUF:                                       ";
    QByteArray str = result.toString().toUtf8();
    switch (result.code) {
        case 0x80: // ACK
            break;
        case 0xff: { // NAK. This mostly means transmission error, not command failed.
            QString logMsg;
            QTextStream(&logMsg) << prefix << "NAK: for token=" << result.token
                << " ERROR: " << errorMessage(result.data.at(0)) << ' ' << str;
            logMessage(logMsg, LogError);
            break;
        }
        case TrkNotifyStopped: {  // 0x90 Notified Stopped
            // 90 01   78 6a 40 40   00 00 07 23   00 00 07 24  00 00
            showMessage(_("RESET SNAPSHOT (NOTIFY STOPPED)"));
            MEMORY_DEBUG("WE STOPPED");
            m_snapshot.reset();
            MEMORY_DEBUG("  AFTER CLEANING: " << m_snapshot.memory.size() << " BLOCKS LEFT");
            QString reason;
            uint addr;
            uint pid;
            uint tid;
            trk::Launcher::parseNotifyStopped(result.data, &pid, &tid, &addr, &reason);
            const QString msg = trk::Launcher::msgStopped(pid, tid, addr, reason);
            // Unknown thread: Add.
            m_session.tid = tid;
            if (m_snapshot.indexOfThread(tid) == -1)
                m_snapshot.addThread(tid);
            m_snapshot.setThreadState(tid, reason);

            logMessage(prefix + msg);
            showMessage(msg, LogMisc);
            sendTrkAck(result.token);
            if (addr) {
                // Todo: Do not send off GdbMessages if a synced gdb
                // query is pending, queue instead
                if (m_running) {
                    m_running = false;
                }
            } else {
                logMessage(QLatin1String("Ignoring stop at 0"));
            }

#            if 1
            // We almost always need register values, so get them
            // now before informing gdb about the stop.s
            const int signalNumber = reason.contains(QLatin1String("exception"), Qt::CaseInsensitive)
                                     || reason.contains(QLatin1String("panic"), Qt::CaseInsensitive) ?
                        gdbServerSignalSegfault : gdbServerSignalTrap;
            sendTrkMessage(0x12,
                TrkCB(handleAndReportReadRegistersAfterStop),
                Launcher::readRegistersMessage(m_session.pid, m_session.tid), signalNumber);
#            else
            // As a source-line step typically consists of
            // several instruction steps, better avoid the multiple
            // roundtrips through TRK in favour of an additional
            // roundtrip through gdb. But gdb will ask for all registers.
#                if 1
                sendGdbServerMessage("S05", "Target stopped");
#                else
                QByteArray ba = "T05";
                appendRegister(&ba, RegisterPSGdb, addr);
                sendGdbServerMessage(ba, "Registers");
#                endif
#            endif
            break;
        }
        case TrkNotifyException: { // 0x91 Notify Exception (obsolete)
            showMessage(_("RESET SNAPSHOT (NOTIFY EXCEPTION)"));
            m_snapshot.reset();
            logMessage(prefix + "NOTE: EXCEPTION  " + str, AppError);
            sendTrkAck(result.token);
            break;
        }
        case 0x92: { //
            showMessage(_("RESET SNAPSHOT (NOTIFY INTERNAL ERROR)"));
            m_snapshot.reset();
            logMessage(prefix + "NOTE: INTERNAL ERROR: " + str, LogError);
            sendTrkAck(result.token);
            break;
        }

        // target->host OS notification
        case 0xa0: { // Notify Created
            // Sending this ACK does not seem to make a difference. Why?
            //sendTrkAck(result.token);
            m_snapshot.resetMemory();
            const char *data = result.data.data();
            const trk::byte error = result.data.at(0);
            // type: 1 byte; for dll item, this value is 2.
            const trk::byte type = result.data.at(1);
            const uint tid = extractInt(data + 6);
            const Library lib = Library(result);
            m_session.libraries.push_back(lib);
            m_session.modules += QString::fromAscii(lib.name);
            QString logMsg;
            QTextStream str(&logMsg);
            str << prefix << " NOTE: LIBRARY LOAD: token=" << result.token;
            if (error)
                str << " ERROR: " << int(error);
            str << " TYPE: " << int(type) << " PID: " << lib.pid << " TID:   " <<  tid;
            str << " CODE: " << hexxNumber(lib.codeseg);
            str << " DATA: " << hexxNumber(lib.dataseg);
            str << " NAME: '" << lib.name << '\'';
            if (tid && tid != unsigned(-1) && m_snapshot.indexOfThread(tid) == -1)
                m_snapshot.addThread(tid);
            logMessage(logMsg);
            // Load local symbol file into gdb provided there is one
            if (lib.codeseg) {
                const QString localSymFileName = Symbian::localSymFileForLibrary(lib.name, m_symbolFileFolder);
                if (!localSymFileName.isEmpty()) {
                    showMessage(Symbian::msgLoadLocalSymFile(localSymFileName, lib.name, lib.codeseg), LogMisc);
                    m_engine->postCommand(Symbian::symFileLoadCommand(localSymFileName, lib.codeseg, lib.dataseg));
                } // has local sym
            } // code seg

            // This lets gdb trigger a register update etc.
            // With CS gdb 6.4 we get a non-standard $qfDllInfo#7f+ request
            // afterwards, so don't use it for now.
            //sendGdbServerMessage("T05library:;");
/*
            // Causes too much "stopped" (by SIGTRAP) messages that need
            // to be answered by "continue". Auto-continuing each SIGTRAP
            // is not possible as this is also the real message for a user
            // initiated interrupt.
            sendGdbServerMessage("T05load:Name=" + lib.name.toHex()
                + ",TextSeg=" + hexNumber(lib.codeseg)
                + ",DataSeg=" + hexNumber(lib.dataseg) + ';');
*/

            // After 'continue' the very first time after starting debugging
            // a process some library load events are generated, these are
            // actually static dependencies for the process. For these libraries,
            // the thread id is -1 which means the debugger doesn't have
            // to continue. The debugger can safely assume that the
            // thread resumption will be handled by the agent itself.
            if (tid != unsigned(-1))
                sendTrkMessage(0x18, TrkCallback(), trkContinueMessage(m_session.mainTid), "CONTINUE");
            break;
        }
        case 0xa1: { // NotifyDeleted
            const ushort itemType = extractByte(result.data.data() + 1);
            const ushort len = result.data.size() > 12
                ? extractShort(result.data.data() + 10) : ushort(0);
            const QString name = len
                ? QString::fromAscii(result.data.mid(12, len)) : QString();
            if (!name.isEmpty())
                m_session.modules.removeAll(name);
            logMessage(_("%1 %2 UNLOAD: %3")
                .arg(QString::fromAscii(prefix))
                .arg(itemType ? QLatin1String("LIB") : QLatin1String("PROCESS"))
                .arg(name));
            sendTrkAck(result.token);
            if (itemType == 0) {
                sendGdbServerMessage("W00", "Process exited");
                //sendTrkMessage(0x02, TrkCB(handleDisconnect));
            }
            break;
        }
        case 0xa2: { // NotifyProcessorStarted
            logMessage(prefix + "NOTE: PROCESSOR STARTED: " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0xa6: { // NotifyProcessorStandby
            logMessage(prefix + "NOTE: PROCESSOR STANDBY: " + str);
            sendTrkAck(result.token);
            break;
        }
        case 0xa7: { // NotifyProcessorReset
            logMessage(prefix + "NOTE: PROCESSOR RESET: " + str);
            sendTrkAck(result.token);
            break;
        }
        default: {
            logMessage(prefix + "INVALID: " + str, LogError);
            break;
        }
    }
}

void TrkGdbAdapter::handleCpuType(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 03 00  04 00 00 04 00 00 00]
    m_session.cpuMajor = result.data[1];
    m_session.cpuMinor = result.data[2];
    m_session.bigEndian = result.data[3];
    m_session.defaultTypeSize = result.data[4];
    m_session.fpTypeSize = result.data[5];
    m_session.extended1TypeSize = result.data[6];
    //m_session.extended2TypeSize = result.data[6];
    QString logMsg;
    QTextStream(&logMsg) << "HANDLE CPU TYPE: CPU=" << m_session.cpuMajor << '.'
        << m_session.cpuMinor << " bigEndian=" << m_session.bigEndian
        << " defaultTypeSize=" << m_session.defaultTypeSize
        << " fpTypeSize=" << m_session.fpTypeSize
        << " extended1TypeSize=" <<  m_session.extended1TypeSize;
    logMessage(logMsg);
}

void TrkGdbAdapter::handleDeleteProcess(const TrkResult &result)
{
    Q_UNUSED(result);
    logMessage("Inferior process killed");
    //sendTrkMessage(0x01, TrkCB(handleDeleteProcess2)); // Ping
    sendTrkMessage(0x02, TrkCB(handleDeleteProcess2)); // Disconnect
}

void TrkGdbAdapter::handleDeleteProcess2(const TrkResult &result)
{
    Q_UNUSED(result);
    QString msg = QString::fromLatin1("App TRK disconnected");

    const bool emergencyShutdown = m_gdbProc.state() != QProcess::Running;
    if (emergencyShutdown)
        msg += QString::fromLatin1(" (emergency shutdown");
    logMessage(msg);
    if (emergencyShutdown) {
        cleanup();
        m_engine->notifyAdapterShutdownOk();
    } else {
        sendGdbServerAck();
        sendGdbServerMessage("", "process killed");
    }
}

void TrkGdbAdapter::handleReadRegisters(const TrkResult &result)
{
    logMessage("       REGISTER RESULT: " + result.toString());
    // [80 0B 00   00 00 00 00   C9 24 FF BC   00 00 00 00   00
    //  60 00 00   00 00 00 00   78 67 79 70   00 00 00 00   00...]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString(), LogError);
        return;
    }
    const char *data = result.data.data() + 1; // Skip ok byte
    uint *registers = m_snapshot.registers(m_session.tid);
    QTC_ASSERT(registers, return;)
    for (int i = 0; i < RegisterCount; ++i)
        registers[i] = extractInt(data + 4 * i);
    m_snapshot.setRegistersValid(m_session.tid, true);
}

void TrkGdbAdapter::handleWriteRegister(const TrkResult &result)
{
    logMessage("       RESULT: " + result.toString() + result.cookie.toString());
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString(), LogError);
        sendGdbServerMessage("E01");
        return;
    }
    sendGdbServerMessage("OK");
}

void TrkGdbAdapter::reportRegisters()
{
    const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
    QTC_ASSERT(threadIndex != -1, return);
    const Symbian::Thread &thread = m_snapshot.threadInfo.at(threadIndex);
    sendGdbServerMessage(thread.gdbReportRegisters(), thread.gdbRegisterLogMessage(m_verbose));
}

void TrkGdbAdapter::handleAndReportReadRegisters(const TrkResult &result)
{
    handleReadRegisters(result);
    reportRegisters();
}

void TrkGdbAdapter::handleAndReportReadRegister(const TrkResult &result)
{
    handleReadRegisters(result);
    const uint registerNumber = result.cookie.toUInt();
    const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
    QTC_ASSERT(threadIndex != -1, return);
    const Symbian::Thread &thread = m_snapshot.threadInfo.at(threadIndex);
    sendGdbServerMessage(thread.gdbReportSingleRegister(registerNumber), thread.gdbSingleRegisterLogMessage(registerNumber));
}

void TrkGdbAdapter::handleAndReportReadRegistersAfterStop(const TrkResult &result)
{
    handleReadRegisters(result);
    const bool reportThread = m_session.tid != m_session.mainTid;
    const int signalNumber = result.cookie.isValid() ? result.cookie.toInt() : int(gdbServerSignalTrap);
    sendGdbServerMessage(m_snapshot.gdbStopMessage(m_session.tid, signalNumber, reportThread),
                         "Stopped with registers in thread " + QByteArray::number(m_session.tid, 16));
}

static QString msgMemoryReadError(int code, uint addr, uint len = 0)
{
    const QString lenS = len ? QString::number(len) : QLatin1String("<unknown>");
    return _("Memory read error %1 at: 0x%2 %3")
        .arg(code).arg(addr, 0 ,16).arg(lenS);
}

void TrkGdbAdapter::handleReadMemoryBuffered(const TrkResult &result)
{
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n", LogError);
    const MemoryRange range = result.cookie.value<MemoryRange>();
    MEMORY_DEBUG("HANDLE READ MEMORY ***BUFFERED*** FOR " << range);
    if (const int errorCode = result.errorCode()) {
        logMessage(_("TEMPORARY: ") + msgMemoryReadError(errorCode, range.from));
        logMessage(_("RETRYING UNBUFFERED"));
        // FIXME: This does not handle large requests properly.
        sendTrkMessage(0x10, TrkCB(handleReadMemoryUnbuffered),
            trkReadMemoryMessage(range), QVariant::fromValue(range));
        return;
    }
    const QByteArray ba = result.data.mid(3);
    MEMORY_DEBUG("INSERT KNOWN MEMORY RANGE: " << range << m_snapshot.memory.size() << " BLOCKS");
    m_snapshot.insertMemory(range, ba);
    tryAnswerGdbMemoryRequest(true);
}

void TrkGdbAdapter::handleReadMemoryUnbuffered(const TrkResult &result)
{
    if (extractShort(result.data.data() + 1) + 3 != result.data.size())
        logMessage("\n BAD MEMORY RESULT: " + result.data.toHex() + "\n", LogError);
    const MemoryRange range = result.cookie.value<MemoryRange>();
    MEMORY_DEBUG("HANDLE READ MEMORY UNBUFFERED FOR " << range);
    if (const int errorCode = result.errorCode()) {
        logMessage(_("TEMPORARY: ") + msgMemoryReadError(errorCode, range.from));
        logMessage(_("RETRYING UNBUFFERED"));
#if 1
        const QByteArray ba = "E20";
        sendGdbServerMessage(ba, msgMemoryReadError(32, range.from).toLatin1());
#else
        // emit bogus data to make Python happy
        MemoryRange wanted = m_snapshot.wantedMemory;
        qDebug() << "SENDING BOGUS DATA FOR " << wanted;
        m_snapshot.insertMemory(wanted, QByteArray(wanted.size(), 0xa5));
        tryAnswerGdbMemoryRequest(false);
#endif
        return;
    }
    const QByteArray ba = result.data.mid(3);
    m_snapshot.insertMemory(range, ba);
    MEMORY_DEBUG("INSERT KNOWN MEMORY RANGE: " << range << m_snapshot.memory.size() << " BLOCKS");
    tryAnswerGdbMemoryRequest(false);
}

void TrkGdbAdapter::tryAnswerGdbMemoryRequest(bool buffered)
{
    //logMessage("TRYING TO ANSWER MEMORY REQUEST ");

    MemoryRange wanted = m_snapshot.wantedMemory;
    MemoryRange needed = m_snapshot.wantedMemory;
    MEMORY_DEBUG("WANTED: " << wanted);
    Snapshot::Memory::const_iterator it = m_snapshot.memory.begin();
    Snapshot::Memory::const_iterator et = m_snapshot.memory.end();
    for ( ; it != et; ++it) {
        MEMORY_DEBUG("   NEEDED STEP: " << needed);
        needed -= it.key();
    }
    MEMORY_DEBUG("NEEDED FINAL: " << needed);

    if (needed.to == 0) {
        // FIXME: need to combine chunks first.

        // All fine. Send package to gdb.
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it) {
            if (it.key().from <= wanted.from && wanted.to <= it.key().to) {
                int offset = wanted.from - it.key().from;
                int len = wanted.to - wanted.from;
                QByteArray ba = it.value().mid(offset, len);
                sendGdbServerMessage(ba.toHex(),
                                     m_snapshot.memoryReadLogMessage(wanted.from, m_session.tid, m_verbose, ba));
                return;
            }
        }
        // Happens when chunks are not combined
        QTC_ASSERT(false, /**/);
        showMessage("CHUNKS NOT COMBINED");
#        ifdef MEMORY_DEBUG
        qDebug() << "CHUNKS NOT COMBINED";
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it)
            qDebug() << hexNumber(it.key().from) << hexNumber(it.key().to);
        qDebug() << "WANTED" << wanted.from << wanted.to;
#        endif
        sendGdbServerMessage("E22", "");
        return;
    }

    MEMORY_DEBUG("NEEDED AND UNSATISFIED: " << needed);
    if (buffered) {
        uint blockaddr = (needed.from / MemoryChunkSize) * MemoryChunkSize;
        logMessage(_("Requesting buffered memory %1 bytes from 0x%2")
            .arg(MemoryChunkSize).arg(blockaddr, 0, 16));
        MemoryRange range(blockaddr, blockaddr + MemoryChunkSize);
        MEMORY_DEBUG("   FETCH BUFFERED MEMORY : " << range);
        sendTrkMessage(0x10, TrkCB(handleReadMemoryBuffered),
            trkReadMemoryMessage(range),
            QVariant::fromValue(range));
    } else { // Unbuffered, direct requests
        int len = needed.to - needed.from;
        logMessage(_("Requesting unbuffered memory %1 bytes from 0x%2")
            .arg(len).arg(needed.from, 0, 16));
        MEMORY_DEBUG("   FETCH UNBUFFERED MEMORY : " << needed);
        sendTrkMessage(0x10, TrkCB(handleReadMemoryUnbuffered),
            trkReadMemoryMessage(needed),
            QVariant::fromValue(needed));
    }
}

/*
void TrkGdbAdapter::reportReadMemoryBuffered(const TrkResult &result)
{
    const MemoryRange range = result.cookie.value<MemoryRange>();
    // Gdb accepts less memory according to documentation.
    // Send E on complete failure.
    QByteArray ba;
    uint blockaddr = (range.from / MemoryChunkSize) * MemoryChunkSize;
    for (; blockaddr < addr + len; blockaddr += MemoryChunkSize) {
        const Snapshot::Memory::const_iterator it = m_snapshot.memory.constFind(blockaddr);
        if (it == m_snapshot.memory.constEnd())
            break;
        ba.append(it.value());
    }
    const int previousChunkOverlap = addr % MemoryChunkSize;
    if (previousChunkOverlap != 0 && ba.size() > previousChunkOverlap)
        ba.remove(0, previousChunkOverlap);
    if (ba.size() > int(len))
        ba.truncate(len);

    if (ba.isEmpty()) {
        ba = "E20";
        sendGdbServerMessage(ba, msgMemoryReadError(32, addr, len).toLatin1());
    } else {
        sendGdbServerMessage(ba.toHex(), memoryReadLogMessage(addr, len, ba));
    }
}
*/

void TrkGdbAdapter::handleStep(const TrkResult &result)
{
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString() + " in handleStep", LogError);

        // Try fallback with Continue.
        showMessage("FALLBACK TO 'CONTINUE'");
        trkContinueAll("Step failed");
        //sendGdbServerMessage("S05", "Stepping finished");

        // Doing nothing as below does not work as gdb seems to insist on
        // making some progress through a 'step'.
        //sendTrkMessage(0x12,
        //    TrkCB(handleAndReportReadRegistersAfterStop),
        //    Launcher::readRegistersMessage(m_session.pid, m_session.tid));
        return;
    }
    // The gdb server response is triggered later by the Stop Reply packet.
    logMessage("STEP FINISHED " + currentTime());
}

void TrkGdbAdapter::handleAndReportSetBreakpoint(const TrkResult &result)
{
    //---TRK------------------------------------------------------
    //  Command: 0x80 Acknowledge
    //    Error: 0x00
    // [80 09 00 00 00 00 0A]
    if (result.errorCode()) {
        logMessage("ERROR WHEN SETTING BREAKPOINT: " + result.errorString(), LogError);
        sendGdbServerMessage("E21");
        return;
    }
    uint bpnr = extractInt(result.data.data() + 1);
    uint addr = result.cookie.toUInt();
    m_session.addressToBP[addr] = bpnr;
    logMessage("SET BREAKPOINT " + hexxNumber(bpnr) + " "
         + stringFromArray(result.data.data()));
    sendGdbServerMessage("OK");
    //sendGdbServerMessage("OK");
}

void TrkGdbAdapter::handleClearBreakpoint(const TrkResult &result)
{
    logMessage("CLEAR BREAKPOINT ");
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString(), LogError);
        //return;
    }
    sendGdbServerMessage("OK");
}

void TrkGdbAdapter::handleSupportMask(const TrkResult &result)
{
    const char *data = result.data.data();
    QByteArray str;
    for (int i = 0; i < 32; ++i) {
        //str.append("  [" + formatByte(data[i]) + "]: ");
        for (int j = 0; j < 8; ++j)
        if (data[i] & (1 << j))
            str.append(QByteArray::number(i * 8 + j, 16));
    }
    logMessage("SUPPORTED: " + str);
 }

void TrkGdbAdapter::handleTrkVersionsStartGdb(const TrkResult &result)
{
    QString logMsg;
    QTextStream str(&logMsg);
    str << "Versions: ";
    if (result.data.size() >= 5) {
        str << "App TRK version " << int(result.data.at(1)) << '.'
            << int(result.data.at(2))
            << ", TRK protocol version " << int(result.data.at(3))
             << '.' << int(result.data.at(4));
    }
    logMessage(logMsg);
    // As we are called from the TrkDevice handler, do not lock up when shutting
    // down the device in case of gdb launch errors.
    QTimer::singleShot(0, this, SLOT(slotStartGdb()));
}

void TrkGdbAdapter::slotStartGdb()
{
    QStringList gdbArgs;
    gdbArgs.append(QLatin1String("--nx")); // Do not read .gdbinit file
    if (!m_engine->startGdb(gdbArgs, QString(), QString())) {
        cleanup();
        return;
    }
    m_engine->handleAdapterStarted();
}

void TrkGdbAdapter::handleDisconnect(const TrkResult & /*result*/)
{
    logMessage(QLatin1String("App TRK disconnected"));
}

void TrkGdbAdapter::readMemory(uint addr, uint len, bool buffered)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device
    if (m_verbose > 2)
        logMessage(_("readMemory %1 bytes from 0x%2 blocksize=%3")
            .arg(len).arg(addr, 0, 16).arg(MemoryChunkSize));

    m_snapshot.wantedMemory = MemoryRange(addr, addr + len);
    tryAnswerGdbMemoryRequest(buffered);
}

void TrkGdbAdapter::writeMemory(uint addr, const QByteArray &data)
{
    Q_ASSERT(data.size() < (2 << 16));
    if (m_verbose > 2) {
        logMessage(_("writeMemory %1 bytes from 0x%2 blocksize=%3 data=%4")
            .arg(data.size()).arg(addr, 0, 16).arg(MemoryChunkSize).arg(QString::fromLatin1(data.toHex())));
    }

    sendTrkMessage(0x11, TrkCB(handleWriteMemory),
        trkWriteMemoryMessage(addr, data));
}

void TrkGdbAdapter::handleWriteMemory(const TrkResult &result)
{
    logMessage("       RESULT: " + result.toString() + result.cookie.toString());
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString(), LogError);
        sendGdbServerMessage("E01");
        return;
    }
    sendGdbServerMessage("OK");
}

void TrkGdbAdapter::interruptInferior()
{
    sendTrkMessage(0x1a, TrkCallback(), trkInterruptMessage(), "Interrupting...");
}

void TrkGdbAdapter::trkDeviceRemoved(const SymbianUtils::SymbianDevice &dev)
{
    if (state() != DebuggerNotReady && !m_trkDevice.isNull() && m_trkDevice->port() == dev.portName()) {
        const QString message = QString::fromLatin1("Device '%1' has been disconnected.").arg(dev.friendlyName());
        logMessage(message);
        m_engine->handleAdapterCrashed(message);
    }
}

bool TrkGdbAdapter::initializeDevice(const QString &remoteChannel, QString *errorMessage)
{
    if (remoteChannel.isEmpty()) {
        *errorMessage = tr("Port specification missing.");
        return false;
    }
    // Run config: Acquire from device manager.
    m_trkDevice = SymbianUtils::SymbianDeviceManager::instance()
        ->acquireDevice(remoteChannel);
    if (m_trkDevice.isNull()) {
        *errorMessage = tr("Unable to acquire a device on '%1'. It appears to be in use.").arg(remoteChannel);
        return false;
    }
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(const SymbianUtils::SymbianDevice)),
            this, SLOT(trkDeviceRemoved(SymbianUtils::SymbianDevice)));
    connect(m_trkDevice.data(), SIGNAL(messageReceived(trk::TrkResult)),
            this, SLOT(handleTrkResult(trk::TrkResult)));
    connect(m_trkDevice.data(), SIGNAL(error(QString)),
            this, SLOT(handleTrkError(QString)));
    connect(m_trkDevice.data(), SIGNAL(logMessage(QString)),
            this, SLOT(trkLogMessage(QString)));
    m_trkDevice->setVerbose(m_verbose);

    // Prompt the user to start communication
    const trk::PromptStartCommunicationResult src =
            S60DebuggerBluetoothStarter::startCommunication(m_trkDevice,
                                                            0, errorMessage);
    switch (src) {
    case trk::PromptStartCommunicationConnected:
        break;
    case trk::PromptStartCommunicationCanceled:
        errorMessage->clear();
        return false;
    case trk::PromptStartCommunicationError:
        return false;
    }
    return true;
}

void TrkGdbAdapter::startAdapter()
{
    m_snapshot.fullReset();

    // Retrieve parameters
    const DebuggerStartParameters &parameters = startParameters();
    m_remoteExecutable = parameters.executable;
    m_remoteArguments = parameters.processArgs;
    m_symbolFile = parameters.symbolFileName;
    if (!m_symbolFile.isEmpty())
        m_symbolFileFolder = QFileInfo(m_symbolFile).absolutePath();
    QString remoteChannel = parameters.remoteChannel;
    // FIXME: testing hack, remove!
    if (m_remoteArguments.startsWith(__("@sym@ "))) {
        QStringList pa = Utils::QtcProcess::splitArgs(m_remoteArguments);
        remoteChannel = pa.at(1);
        m_remoteExecutable = pa.at(2);
        m_symbolFile = pa.at(3);
        m_remoteArguments.clear();
    }
    // Unixish gdbs accept only forward slashes
    m_symbolFile.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // Start
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    logMessage(QLatin1String("### Starting TrkGdbAdapter"));

    // Prompt the user to start communication
    QString message;
    if (!initializeDevice(remoteChannel, &message)) {
        if (message.isEmpty()) {
            m_engine->handleAdapterStartFailed(QString(), QString());
        } else {
            logMessage(message, LogError);
            m_engine->handleAdapterStartFailed(message, QString());
        }
        return;
    }

    QTC_ASSERT(m_gdbServer == 0, delete m_gdbServer);
    QTC_ASSERT(m_gdbConnection == 0, m_gdbConnection = 0);
    m_gdbServer = new QTcpServer(this);

    if (!m_gdbServer->listen(QHostAddress(gdbServerIP()), gdbServerPort())) {
        QString msg = QString("Unable to start the gdb server at %1: %2.")
            .arg(m_gdbServerName).arg(m_gdbServer->errorString());
        logMessage(msg, LogError);
        m_engine->handleAdapterStartFailed(msg, QString());
        return;
    }

    logMessage(QString("Gdb server running on %1.\nLittle endian assumed.")
        .arg(m_gdbServerName));

    connect(m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));

    m_trkDevice->sendTrkInitialPing();
    sendTrkMessage(0x02); // Disconnect, as trk might be still connected
    sendTrkMessage(0x01); // Connect
    sendTrkMessage(0x05, TrkCB(handleSupportMask));
    sendTrkMessage(0x06, TrkCB(handleCpuType));
    sendTrkMessage(0x04, TrkCB(handleTrkVersionsStartGdb)); // Versions
}

void TrkGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    sendTrkMessage(0x40, TrkCB(handleCreateProcess),
                   trk::Launcher::startProcessMessage(m_remoteExecutable, m_remoteArguments));
}

void TrkGdbAdapter::handleCreateProcess(const TrkResult &result)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    //  40 00 00]
    //logMessage("       RESULT: " + result.toString());
    // [80 08 00   00 00 01 B5   00 00 01 B6   78 67 40 00   00 40 00 00]
    if (result.errorCode()) {
        logMessage("ERROR: " + result.errorString(), LogError);
        QString msg = _("Cannot start executable \"%1\" on the device:\n%2")
            .arg(m_remoteExecutable).arg(result.errorString());
        // Delay cleanup as not to close a trk device from its read handler,
        // which blocks.
        emitDelayedInferiorSetupFailed(msg);
        return;
    }
    showMessage(_("RESET SNAPSHOT (NOTIFY CREATED)"));
    m_snapshot.fullReset();
    const char *data = result.data.data();
    m_session.pid = extractInt(data + 1);
    m_session.mainTid = m_session.tid = extractInt(data + 5);
    m_session.codeseg = extractInt(data + 9);
    m_session.dataseg = extractInt(data + 13);
    m_snapshot.addThread(m_session.mainTid);
    const QString startMsg =
        tr("Process started, PID: 0x%1, thread id: 0x%2, "
           "code segment: 0x%3, data segment: 0x%4.")
             .arg(m_session.pid, 0, 16).arg(m_session.tid, 0, 16)
             .arg(m_session.codeseg, 0, 16).arg(m_session.dataseg, 0, 16);
    logMessage(startMsg, LogMisc);
    // 26.8.2010: When paging occurs in S^3, bogus starting ROM addresses
    // like 0x500000 or 0x40000 are reported. Warn about symbol resolution errors.
    // Code duplicated in TcfTrkAdapter. @TODO: Hopefully fixed in future TRK versions.
    if ((m_session.codeseg  & 0xFFFFF) == 0) {
        const QString warnMessage = tr("The reported code segment address (0x%1) might be invalid. Symbol resolution or setting breakoints may not work.").
                                    arg(m_session.codeseg, 0, 16);
        logMessage(warnMessage, LogError);
    }

    m_engine->postCommand("set gnutarget arm-none-symbianelf");

    const QByteArray symbolFile = m_symbolFile.toLocal8Bit();
    if (symbolFile.isEmpty()) {
        logMessage(_("WARNING: No symbol file available."), LogError);
    } else {
        // Does not seem to be necessary anymore.
        // FIXME: Startup sequence can be streamlined now as we do not
        // have to wait for the TRK startup to learn the load address.
        //m_engine->postCommand("add-symbol-file \"" + symbolFile + "\" "
        //    + QByteArray::number(m_session.codeseg));
        m_engine->postCommand("symbol-file \"" + symbolFile + "\"");
    }
    foreach(const QByteArray &s, Symbian::gdbStartupSequence())
        m_engine->postCommand(s);
    //m_engine->postCommand("set remotelogfile /tmp/gdb-remotelog");
    //m_engine->postCommand("set debug remote 1"); // FIXME: Make an option.
    m_engine->postCommand("target remote " + gdbServerName().toLatin1(),
        CB(handleTargetRemote));
}

void TrkGdbAdapter::handleTargetRemote(const GdbResponse &record)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (record.resultClass == GdbResultDone) {
        m_engine->handleInferiorPrepared();
    } else {
        QString msg = tr("Connecting to TRK server adapter failed:\n")
            + QString::fromLocal8Bit(record.data.findChild("msg").data());
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void TrkGdbAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyEngineRunAndInferiorStopOk();
    m_engine->continueInferiorInternal();
}

//
// AbstractGdbAdapter interface implementation
//

void TrkGdbAdapter::write(const QByteArray &data)
{
    // Write magic packets directly to TRK.
    if (data.startsWith("@#")) {
        QByteArray data1 = data.mid(2);
        if (data1.endsWith(char(10)))
            data1.chop(1);
        if (data1.endsWith(char(13)))
            data1.chop(1);
        if (data1.endsWith(' '))
            data1.chop(1);
        bool ok;
        uint addr = data1.toUInt(&ok, 0);
        qDebug() << "Writing: " << quoteUnprintableLatin1(data1) << addr;
        directStep(addr);
        return;
    }
    if (data.startsWith("@$")) {
        QByteArray ba = QByteArray::fromHex(data.mid(2));
        qDebug() << "Writing: " << quoteUnprintableLatin1(ba);
        if (ba.size() >= 1)
            sendTrkMessage(ba.at(0), TrkCB(handleDirectTrk), ba.mid(1));
        return;
    }
    if (data.startsWith("@@")) {
        // Read data
        sendTrkMessage(0x10, TrkCB(handleDirectWrite1),
                       Launcher::readMemoryMessage(m_session.pid, m_session.tid, m_session.dataseg, 12));
        return;
    }
    m_gdbProc.write(data);
}

uint oldPC;
QByteArray oldMem;
uint scratch;

void TrkGdbAdapter::handleDirectWrite1(const TrkResult &response)
{
    scratch = m_session.dataseg + 512;
    logMessage("DIRECT WRITE1: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        oldMem = response.data.mid(3);
        oldPC = m_snapshot.registerValue(m_session.tid, RegisterPC);
        logMessage("READ MEM: " + oldMem.toHex());
        //qDebug("READ MEM: " + oldMem.toHex());
        QByteArray ba;
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);
        appendByte(&ba, 0xaa);

#if 0
        // Arm:
        //  0:   e51f4004        ldr     r4, [pc, #-4]   ; 4 <.text+0x4>
        appendByte(&ba, 0x04);
        appendByte(&ba, 0x50);  // R5
        appendByte(&ba, 0x1f);
        appendByte(&ba, 0xe5);
#else
        // Thumb:
        // subs  r0, #16
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        // subs  r0, #16
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        //
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
        // subs  r0, #16
        appendByte(&ba, 0x08);
        appendByte(&ba, 0x3b);
#endif

        // Write data
        sendTrkMessage(0x11, TrkCB(handleDirectWrite2),
            trkWriteMemoryMessage(scratch, ba));
    }
}

void TrkGdbAdapter::handleDirectWrite2(const TrkResult &response)
{
    logMessage("DIRECT WRITE2: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Check
        sendTrkMessage(0x10, TrkCB(handleDirectWrite3),
            trk::Launcher::readMemoryMessage(m_session.pid, m_session.tid, scratch, 12));
    }
}

void TrkGdbAdapter::handleDirectWrite3(const TrkResult &response)
{
    logMessage("DIRECT WRITE3: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Set PC
        sendTrkMessage(0x13, TrkCB(handleDirectWrite4),
            trkWriteRegisterMessage(RegisterPC, scratch + 4));
    }
}

void TrkGdbAdapter::handleDirectWrite4(const TrkResult &response)
{
    m_snapshot.setRegisterValue(m_session.tid, RegisterPC, scratch + 4);
return;
    logMessage("DIRECT WRITE4: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        QByteArray ba1;
        appendByte(&ba1, 0x11); // options "step over"
        appendInt(&ba1, scratch + 4);
        appendInt(&ba1, scratch + 4);
        appendInt(&ba1, m_session.pid);
        appendInt(&ba1, m_session.tid);
        sendTrkMessage(0x19, TrkCB(handleDirectWrite5), ba1);
    }
}

void TrkGdbAdapter::handleDirectWrite5(const TrkResult &response)
{
    logMessage("DIRECT WRITE5: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Restore PC
        sendTrkMessage(0x13, TrkCB(handleDirectWrite6),
            trkWriteRegisterMessage(RegisterPC, oldPC));
    }
}

void TrkGdbAdapter::handleDirectWrite6(const TrkResult &response)
{
    logMessage("DIRECT WRITE6: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Restore memory
        sendTrkMessage(0x11, TrkCB(handleDirectWrite7),
            trkWriteMemoryMessage(scratch, oldMem));
    }
}

void TrkGdbAdapter::handleDirectWrite7(const TrkResult &response)
{
    logMessage("DIRECT WRITE7: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Check
        sendTrkMessage(0x10, TrkCB(handleDirectWrite8),
                       trk::Launcher::readMemoryMessage(m_session.pid, m_session.tid,
                                                        scratch, 8));
    }
}

void TrkGdbAdapter::handleDirectWrite8(const TrkResult &response)
{
    logMessage("DIRECT WRITE8: " + response.toString());
    if (const int errorCode = response.errorCode()) {
        logMessage("ERROR: " + response.errorString() + "in handleDirectWrite1", LogError);
    } else {
        // Re-read registers
        sendTrkMessage(0x12,
            TrkCB(handleAndReportReadRegistersAfterStop),
            Launcher::readRegistersMessage(m_session.pid, m_session.tid));
    }
}

void TrkGdbAdapter::handleDirectWrite9(const TrkResult &response)
{
    logMessage("DIRECT WRITE9: " + response.toString());
}

void TrkGdbAdapter::handleDirectTrk(const TrkResult &result)
{
    logMessage("HANDLE DIRECT TRK: " + stringFromArray(result.data));
}

void TrkGdbAdapter::directStep(uint addr)
{
    // Write PC:
    qDebug() << "ADDR: " << addr;
    oldPC = m_snapshot.registerValue(m_session.tid, RegisterPC);
    m_snapshot.setRegisterValue(m_session.tid, RegisterPC, addr);
    QByteArray ba = trkWriteRegisterMessage(RegisterPC, addr);
    sendTrkMessage(0x13, TrkCB(handleDirectStep1), ba, "Write PC");
}

void TrkGdbAdapter::handleDirectStep1(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP1: " + stringFromArray(result.data));
    QByteArray ba;
    const uint pc = oldPC = m_snapshot.registerValue(m_session.tid, RegisterPC);
    appendByte(&ba, 0x11); // options "step over"
    appendInt(&ba, pc);
    appendInt(&ba, pc);
    appendInt(&ba, m_session.pid);
    appendInt(&ba, m_session.tid);
    sendTrkMessage(0x19, TrkCB(handleDirectStep2), ba, "Direct step");
}

void TrkGdbAdapter::handleDirectStep2(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP2: " + stringFromArray(result.data));
    m_snapshot.setRegisterValue(m_session.tid, RegisterPC, oldPC);
        QByteArray ba = trkWriteRegisterMessage(RegisterPC, oldPC);
    sendTrkMessage(0x13, TrkCB(handleDirectStep3), ba, "Write PC");
}

void TrkGdbAdapter::handleDirectStep3(const TrkResult &result)
{
    logMessage("HANDLE DIRECT STEP2: " + stringFromArray(result.data));
}

void TrkGdbAdapter::cleanup()
{
    if (!m_trkDevice.isNull()) {
        m_trkDevice->close();
        m_trkDevice->disconnect(this);
        SymbianUtils::SymbianDeviceManager::instance()->releaseDevice(m_trkDevice->port());
        m_trkDevice = TrkDevicePtr();
    }

    delete m_gdbServer;
    m_gdbServer = 0;
}

void TrkGdbAdapter::shutdownInferior()
{
    m_engine->defaultInferiorShutdown("kill");
}

void TrkGdbAdapter::shutdownAdapter()
{
    if (m_gdbProc.state() == QProcess::Running) {
        cleanup();
        m_engine->notifyAdapterShutdownOk();
    } else {
        // Something is wrong, gdb crashed. Kill debuggee (see handleDeleteProcess2)
        if (m_trkDevice->isOpen()) {
            logMessage("Emergency shutdown of TRK", LogError);
            trkKill();
        }
    }
}

void TrkGdbAdapter::trkReloadRegisters()
{
    m_snapshot.syncRegisters(m_session.tid, m_engine->registerHandler());
}

void TrkGdbAdapter::trkReloadThreads()
{
    m_snapshot.syncThreads(m_engine->threadsHandler());
}

} // namespace Internal
} // namespace Debugger
