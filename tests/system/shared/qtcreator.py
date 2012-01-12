import platform;
import shutil;
import os;
import glob;
import atexit;
import codecs;
import subprocess;
import errno;
from datetime import datetime,timedelta;

srcPath = ''
SettingsPath = ''
tmpSettingsDir = ''
testSettings.logScreenshotOnFail = True
testSettings.logScreenshotOnError = True

source("../../shared/classes.py")
source("../../shared/utils.py")
source("../../shared/build_utils.py")
source("../../shared/project.py")
source("../../shared/editor_utils.py")
source("../../shared/project_explorer.py")
source("../../shared/hook_utils.py")

def waitForCleanShutdown(timeOut=10):
    appCtxt = currentApplicationContext()
    shutdownDone = (str(appCtxt)=="")
    if platform.system() in ('Windows','Microsoft'):
        endtime = datetime.utcnow() + timedelta(seconds=timeOut)
        while not shutdownDone:
            # following work-around because os.kill() works for win not until python 2.7
            if appCtxt.pid==-1:
                break
            tasks = subprocess.Popen("tasklist /FI \"PID eq %d\"" % appCtxt.pid, shell=True,stdout=subprocess.PIPE)
            output = tasks.communicate()[0]
            tasks.stdout.close()
            if (output=="INFO: No tasks are running which match the specified criteria."
                or output=="" or output.find("ERROR")==0):
                shutdownDone=True
            if not shutdownDone and datetime.utcnow() > endtime:
                break
    else:
        endtime = datetime.utcnow() + timedelta(seconds=timeOut)
        while not shutdownDone:
            try:
                os.kill(appCtxt.pid,0)
            except OSError, err:
                if err.errno == errno.EPERM or err.errno == errno.ESRCH:
                    shutdownDone=True
            if not shutdownDone and datetime.utcnow() > endtime:
                break

def __removeTmpSettingsDir__():
    waitForCleanShutdown()
    deleteDirIfExists(os.path.dirname(os.path.dirname(tmpSettingsDir)))

if platform.system() in ('Windows', 'Microsoft'):
    sdkPath = "C:\\QtSDK"
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="\\..\\..\\settings\\windows"
    defaultQtVersion = "Qt 4.7.4 for Desktop - MinGW 4.4 (Qt SDK)"
else:
    sdkPath = os.path.expanduser("~/QtSDK")
    cwd = os.getcwd()       # current dir is directory holding qtcreator.py
    cwd+="/../../settings/unix"
    defaultQtVersion = "Desktop Qt 4.7.4 for GCC (Qt SDK)"
srcPath = os.getenv("SYSTEST_SRCPATH", sdkPath + "/src")

# the following only doesn't work if the test ends in an exception
if os.getenv("SYSTEST_NOSETTINGSPATH") != "1":
    cwd = os.path.abspath(cwd)
    tmpSettingsDir = tempDir()
    tmpSettingsDir = os.path.abspath(tmpSettingsDir+"/settings")
    shutil.copytree(cwd, tmpSettingsDir)
    atexit.register(__removeTmpSettingsDir__)
    SettingsPath = ' -settingspath "%s"' % tmpSettingsDir
