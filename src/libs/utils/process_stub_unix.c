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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#ifdef __sun
# define PT_TRACE_ME 0
# define PT_DETACH 7
#else
# include <sys/ptrace.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* For OpenBSD */
#ifndef EPROTO
# define EPROTO EINVAL
#endif

extern char **environ;

static int qtcFd;
static char *sleepMsg;

static void __attribute__((noreturn)) doExit(int code)
{
    tcsetpgrp(0, getpid());
    puts(sleepMsg);
    fgets(sleepMsg, 2, stdin); /* Minimal size to make it wait */
    exit(code);
}

static void sendMsg(const char *msg, int num)
{
    int pidStrLen;
    int ioRet;
    char pidStr[64];

    pidStrLen = sprintf(pidStr, msg, num);
    if ((ioRet = write(qtcFd, pidStr, pidStrLen)) != pidStrLen) {
        fprintf(stderr, "Cannot write to creator comm socket: %s\n",
                        (ioRet < 0) ? strerror(errno) : "short write");
        doExit(3);
    }
}

enum {
    ArgCmd = 0,
    ArgAction,
    ArgSocket,
    ArgMsg,
    ArgDir,
    ArgEnv,
    ArgExe
};

/* syntax: $0 {"run"|"debug"} <pid-socket> <continuation-msg> <workdir> <env-file> <exe> <args...> */
/* exit codes: 0 = ok, 1 = invocation error, 3 = internal error */
int main(int argc, char *argv[])
{
    int errNo;
    int chldPid;
    int chldStatus;
    int chldPipe[2];
    char **env = 0;
    struct sockaddr_un sau;

    if (argc < ArgEnv) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }
    sleepMsg = argv[ArgMsg];

    /* Connect to the master, i.e. Creator. */
    if ((qtcFd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create creator comm socket");
        doExit(3);
    }
    memset(&sau, 0, sizeof(sau));
    sau.sun_family = AF_UNIX;
    strncpy(sau.sun_path, argv[ArgSocket], sizeof(sau.sun_path) - 1);
    if (connect(qtcFd, (struct sockaddr *)&sau, sizeof(sau))) {
        fprintf(stderr, "Cannot connect creator comm socket %s: %s\n", sau.sun_path, strerror(errno));
        doExit(1);
    }

    if (*argv[ArgDir] && chdir(argv[ArgDir])) {
        /* Only expected error: no such file or direcotry */
        sendMsg("err:chdir %d\n", errno);
        return 1;
    }

    if (*argv[ArgEnv]) {
        FILE *envFd;
        char *envdata, *edp;
        long size;
        int count;
        if (!(envFd = fopen(argv[ArgEnv], "r"))) {
            fprintf(stderr, "Cannot read creator env file %s: %s\n",
                    argv[ArgEnv], strerror(errno));
            doExit(1);
        }
        fseek(envFd, 0, SEEK_END);
        size = ftell(envFd);
        rewind(envFd);
        envdata = malloc(size);
        if (fread(envdata, 1, size, envFd) != (size_t)size) {
            perror("Failed to read env file");
            doExit(1);
        }
        fclose(envFd);
        for (count = 0, edp = envdata; edp < envdata + size; ++count)
            edp += strlen(edp) + 1;
        env = malloc((count + 1) * sizeof(char *));
        for (count = 0, edp = envdata; edp < envdata + size; ++count) {
            env[count] = edp;
            edp += strlen(edp) + 1;
        }
        env[count] = 0;
    }

    /* Create execution result notification pipe. */
    if (pipe(chldPipe)) {
        perror("Cannot create status pipe");
        doExit(3);
    }
    /* The debugged program is not supposed to inherit these handles. But we cannot
     * close the writing end before calling exec(). Just handle both ends the same way ... */
    fcntl(chldPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(chldPipe[1], F_SETFD, FD_CLOEXEC);
    switch ((chldPid = fork())) {
        case -1:
            perror("Cannot fork child process");
            doExit(3);
        case 0:
            close(qtcFd);

            /* Put the process into an own process group and make it the foregroud
             * group on this terminal, so it will receive ctrl-c events, etc.
             * This is the main reason for *all* this stub magic in the first place. */
            /* If one of these calls fails, the world is about to end anyway, so
             * don't bother checking the return values. */
            setpgid(0, 0);
            tcsetpgrp(0, getpid());

            /* Get a SIGTRAP after exec() has loaded the new program. */
#ifdef __linux__
            ptrace(PTRACE_TRACEME);
#else
            ptrace(PT_TRACE_ME, 0, 0, 0);
#endif

            if (env)
                environ = env;

            execvp(argv[ArgExe], argv + ArgExe);
            /* Only expected error: no such file or direcotry, i.e. executable not found */
            errNo = errno;
            write(chldPipe[1], &errNo, sizeof(errNo)); /* Only realistic error case is SIGPIPE */
            _exit(0);
        default:
            for (;;) {
                if (wait(&chldStatus) < 0) {
                    perror("Cannot obtain exit status of child process");
                    doExit(3);
                }
                if (WIFSTOPPED(chldStatus)) {
                    /* The child stopped. This can be only the result of ptrace(TRACE_ME). */
                    /* We won't need the notification pipe any more, as we know that
                     * the exec() succeeded. */
                    close(chldPipe[0]);
                    close(chldPipe[1]);
                    chldPipe[0] = -1;
                    /* If we are not debugging, just skip the "handover enabler".
                     * This is suboptimal, as it makes us ignore setuid/-gid bits. */
                    if (!strcmp(argv[ArgAction], "debug")) {
                        /* Stop the child after we detach from it, so we can hand it over to gdb.
                         * If the signal delivery is not queued, things will go awry. It works on
                         * Linux and MacOSX ... */
                        kill(chldPid, SIGSTOP);
                    }
#ifdef __linux__
                    ptrace(PTRACE_DETACH, chldPid, 0, 0);
#else
                    ptrace(PT_DETACH, chldPid, 0, 0);
#endif
                    sendMsg("pid %d\n", chldPid);
                } else if (WIFEXITED(chldStatus)) {
                    /* The child exited normally. */
                    if (chldPipe[0] >= 0) {
                        /* The child exited before being stopped by ptrace(). That can only
                         * mean that the exec() failed. */
                        switch (read(chldPipe[0], &errNo, sizeof(errNo))) {
                            default:
                                /* Read of unknown length. Should never happen ... */
                                errno = EPROTO;
                            case -1:
                                /* Read failed. Should never happen, either ... */
                                perror("Cannot read status from child process");
                                doExit(3);
                            case sizeof(errNo):
                                /* Child telling us the errno from exec(). */
                                sendMsg("err:exec %d\n", errNo);
                                return 3;
                        }
                    }
                    sendMsg("exit %d\n", WEXITSTATUS(chldStatus));
                    doExit(0);
                } else {
                    sendMsg("crash %d\n", WTERMSIG(chldStatus));
                    doExit(0);
                }
            }
            break;
    }
}
