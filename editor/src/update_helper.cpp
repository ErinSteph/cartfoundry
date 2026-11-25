#include "update_model.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QThread>
#include <QTextStream>
using namespace cf::updates;
static bool launch(const QString &root,const QString &job){
 QStringList args{"--update-job",job+"/plan.json"};auto project=readJson(job+"/plan.json")["project"].toString();if(QDir::isAbsolutePath(project)&&QFileInfo(project).isFile())args<<"--"<<project;
#ifdef Q_OS_MACOS
 return QProcess::startDetached("/usr/bin/open",QStringList{"-n",root,"--args"}+args);
#else
 return QProcess::startDetached(executablePath(root),args,root);
#endif
}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);auto args=app.arguments();
 if(args.size()==3&&args[1]=="--verify-archive"){QString error;if(inspectArchive(args[2],packageName(),error))return 0;QTextStream(stderr)<<error<<'\n';return 1;}
 if(args.size()!=3||args[1]!="--plan")return 2;
 QString error;InstallPlan plan;if(!parsePlan(args[2],plan,error))return 2;
 auto result=[&](QString state,QString message){QString ignored;writeJson(plan.job+"/result.json",{{"state",state},{"message",message}},ignored);};
 QLockFile lock(QFileInfo(plan.target).absolutePath()+"/.CartFoundry-update.lock");lock.setStaleLockTime(0);
 if(!lock.tryLock(0)){result("failed","Another updater is already running.");return 1;}
 if(!waitForExit(plan.parentPid,error)){result("failed",error);return 1;}
 QString backup;
 if(!replaceInstallation(plan,backup,error)){result("failed",error);launch(plan.target,plan.job);return 1;}
 bool started=launch(plan.target,plan.job);QElapsedTimer timer;timer.start();
 while(started&&timer.elapsed()<60000){auto receipt=readJson(plan.job+"/confirmed.json");if(receipt["version"]==plan.version.text()&&receipt["target"]==plan.target){result("success",QString());return 0;}if(timer.elapsed()>10000&&!installationIsRunning(plan.target))break;QThread::msleep(200);}
 error=started?"The updated app did not confirm startup.":"The updated app could not be started.";
 QString rollbackError;if(rollbackInstallation(plan,backup,rollbackError)){result("rolled-back",error+" The previous app was restored.");launch(plan.target,plan.job);}else result("unconfirmed",error+" "+rollbackError);
 return 1;
}
