#include "updater.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtConcurrent>
#include <memory>
namespace cf {
using namespace updates;
static QNetworkRequest request(const QUrl &url){QNetworkRequest r(url);r.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);r.setMaximumRedirectsAllowed(5);r.setTransferTimeout(30000);r.setRawHeader("User-Agent",("CartFoundry/"+QCoreApplication::applicationVersion()).toUtf8());r.setRawHeader("Accept-Encoding","identity");r.setRawHeader("Cache-Control","no-cache");return r;}
static QString installedRoot(){
#ifdef Q_OS_MACOS
 return QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../..");
#else
 return QCoreApplication::applicationDirPath();
#endif
}
static QString helperPath(const QString &root){
#ifdef Q_OS_MACOS
 return root+"/Contents/MacOS/cf-update-helper";
#elif defined(Q_OS_WIN)
 return root+"/cf-update-helper.exe";
#else
 return root+"/cf-update-helper";
#endif
}
Updater::Updater(QWidget *w,QNetworkAccessManager *manager,const QString &installationRoot,std::function<QString()> reopen):QObject(w),window(w),network(manager?manager:new QNetworkAccessManager(this)),root(installationRoot.isEmpty()?installedRoot():installationRoot),reopenProject(std::move(reopen)){}
void Updater::addToMenu(QMenu *menu){
 menu->addSeparator();auto *checkAction=menu->addAction("Check for Updates…");checkAction->setObjectName("checkForUpdates");connect(checkAction,&QAction::triggered,this,[this]{check();});
 automaticAction=menu->addAction("Ask about updates on launch");automaticAction->setObjectName("automaticUpdatePrompts");automaticAction->setCheckable(true);automaticAction->setChecked(QSettings().value("updates/automaticPrompts",true).toBool());
 connect(automaticAction,&QAction::toggled,this,[](bool value){QSettings s;s.setValue("updates/automaticPrompts",value);s.sync();});
}
void Updater::start(const QString &planFile){
 auto current=Version::parse(QCoreApplication::applicationVersion());if(!current)return;
 bool attempted=!planFile.isEmpty();
 if(attempted){InstallPlan p;QString error;if(parsePlan(planFile,p,error)&&QDir::cleanPath(p.target)==root){auto result=readJson(p.job+"/result.json");if(result["state"]=="rolled-back"||result["state"]=="failed"){QMessageBox::warning(window,"Update not installed",result["message"].toString());}
  else if(p.version==*current&&validateInstallation(root,*current,error)){writeJson(p.job+"/confirmed.json",{{"version",current->text()},{"target",root}},error);}
 }}
 QSettings settings;observeInstalledVersion(settings,*current);if(automaticAction)automaticAction->setChecked(settings.value("updates/automaticPrompts",true).toBool());
 // Completed staging copies are disposable cache, not the rollback installation.
 QDir cache(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates");
 for(auto entry:cache.entryInfoList({"job-*"},QDir::Dirs|QDir::NoDotAndDotDot)){if(entry.isSymLink()||entry.lastModified().secsTo(QDateTime::currentDateTime())<86400)continue;auto result=readJson(entry.filePath()+"/result.json");if(!result.isEmpty())QDir(entry.filePath()).removeRecursively();}
 if(!attempted&&settings.value("updates/automaticPrompts",true).toBool()&&QFileInfo(helperPath(root)).isFile())QTimer::singleShot(1500,this,[this]{if(!checkedThisLaunch)check(false);});
}
void Updater::finish(const QString &message,bool failure){busy=false;if(progress){progress->close();progress->deleteLater();progress=nullptr;}if(!message.isEmpty()&&manual)QTimer::singleShot(0,this,[this,message,failure]{QMessageBox box(failure?QMessageBox::Warning:QMessageBox::Information,"CartFoundry updates",message,QMessageBox::Ok,window);box.setTextFormat(Qt::PlainText);box.exec();});}
void Updater::check(bool userRequested){
 if(!userRequested&&!QSettings().value("updates/automaticPrompts",true).toBool())return;
 if(busy){manual=manual||userRequested;return;}manual=userRequested;checkedThisLaunch=true;
 if(!QFileInfo(helperPath(root)).isFile()||!QFileInfo(metadataPath(root)).isFile()||QFileInfo(root).fileName()!=packageName()){finish("Automatic updates are available in packaged CartFoundry installations. This development copy is not an installed package.");return;}
 busy=true;auto *reply=network->get(request(QUrl(FeedUrl)));reply->setReadBufferSize(256*1024+1);auto data=std::make_shared<QByteArray>();
 connect(reply,&QNetworkReply::readyRead,this,[reply,data]{*data+=reply->readAll();if(data->size()>256*1024)reply->abort();});
 connect(reply,&QNetworkReply::finished,this,[this,reply,data]{*data+=reply->readAll();reply->deleteLater();if(reply->error()!=QNetworkReply::NoError||reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200){finish("Could not check for updates. Check your connection and try again later.",true);return;}
  QString error;bool available;if(!parseManifest(*data,platformKey(),release,available,error)){finish(error,true);return;}auto current=Version::parse(QCoreApplication::applicationVersion());if(!current||!(*current<release.version)){finish("CartFoundry "+QCoreApplication::applicationVersion()+" is up to date.");return;}
  if(!available){finish("A newer release exists, but no update package has been published for this platform.");return;}probe();
 });
}
void Updater::probe(bool range){
 auto r=request(release.url);if(range)r.setRawHeader("Range","bytes=0-0");auto *reply=range?network->get(r):network->head(r);reply->setReadBufferSize(1024);
 if(range)connect(reply,&QNetworkReply::metaDataChanged,this,[reply]{int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();if(code==200||code==206){reply->setProperty("available",true);reply->abort();}});
 connect(reply,&QNetworkReply::finished,this,[this,reply,range]{int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();bool exists=(reply->error()==QNetworkReply::NoError||reply->property("available").toBool())&&(code==200||code==206);reply->deleteLater();
  if(!range&&(code==405||code==501)){probe(true);return;}if(!exists){finish("The update package is not available for download yet. Try again later.");return;}QTimer::singleShot(0,this,[this]{offer();});
 });
}
void Updater::offer(){
 if(!manual&&!QSettings().value("updates/automaticPrompts",true).toBool()){finish();return;}
 QMessageBox box(QMessageBox::Information,"CartFoundry update","CartFoundry "+release.version.text()+" is available. You have "+QCoreApplication::applicationVersion()+".\n\nDownload and install the update?",QMessageBox::NoButton,window);box.setTextFormat(Qt::PlainText);if(!release.notes.isEmpty())box.setDetailedText(release.notes);
 auto *update=box.addButton("Update",QMessageBox::AcceptRole);box.addButton("Later",QMessageBox::RejectRole);auto *stop=box.addButton("Don’t ask automatically",QMessageBox::RejectRole);box.setDefaultButton(update);box.exec();
 if(box.clickedButton()==stop){QSettings s;s.setValue("updates/automaticPrompts",false);s.sync();if(automaticAction)automaticAction->setChecked(false);finish();}
 else if(box.clickedButton()==update)download();else finish();
}
void Updater::download(){
 auto base=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates";QDir().mkpath(base);QTemporaryDir directory(base+"/job-XXXXXX");if(!directory.isValid()){manual=true;finish("Cannot create the update download folder.",true);return;}directory.setAutoRemove(false);job=directory.path();
 auto file=std::make_shared<QFile>(job+"/download.zip");if(!file->open(QIODevice::WriteOnly|QIODevice::NewOnly)){manual=true;finish(file->errorString(),true);return;}
 progress=new QProgressDialog("Downloading CartFoundry "+release.version.text()+"…","Cancel",0,100,window);progress->setWindowModality(Qt::WindowModal);progress->setMinimumDuration(0);progress->show();
 auto *reply=network->get(request(release.url));reply->setReadBufferSize(256*1024);auto received=std::make_shared<qint64>(0);auto cancelled=std::make_shared<bool>(false);auto writeError=std::make_shared<bool>(false);
 connect(progress,&QProgressDialog::canceled,this,[guard=QPointer<QNetworkReply>(reply),cancelled]{*cancelled=true;if(guard)guard->abort();});
 auto consume=[reply,file,received,writeError,this]{auto bytes=reply->readAll();*received+=bytes.size();if(*received>release.size||file->write(bytes)!=bytes.size()){*writeError=true;reply->abort();}};
 connect(reply,&QNetworkReply::readyRead,this,consume);
 connect(reply,&QNetworkReply::downloadProgress,this,[this](qint64 n,qint64){if(progress)progress->setValue(int(qMin(99LL,n*100/release.size)));});
 QTimer::singleShot(600000,reply,[reply]{if(!reply->isFinished())reply->abort();});
 connect(reply,&QNetworkReply::finished,this,[this,reply,file,received,cancelled,writeError,consume]{consume();file->close();reply->deleteLater();if(*cancelled){finish();QDir(job).removeRecursively();return;}
  if(reply->error()!=QNetworkReply::NoError||*writeError||*received!=release.size){manual=true;finish("The update download failed or was incomplete. The installed app is unchanged.",true);QDir(job).removeRecursively();return;}prepare();
 });
}
void Updater::prepare(){
 if(progress){progress->setLabelText("Verifying and preparing the update…");progress->setRange(0,0);}
 auto cancelled=std::make_shared<bool>(false);if(progress)connect(progress,&QProgressDialog::canceled,this,[cancelled]{*cancelled=true;});
 struct Prepared {QString error,helper;};auto *watcher=new QFutureWatcher<Prepared>(this);auto releaseCopy=release;auto source=root;auto directory=job;auto currentVersion=QCoreApplication::applicationVersion();
 connect(watcher,&QFutureWatcher<Prepared>::finished,this,[this,watcher,cancelled]{auto result=watcher->result();watcher->deleteLater();if(*cancelled){finish();QDir(job).removeRecursively();return;}if(!result.error.isEmpty()){manual=true;finish(result.error,true);QDir(job).removeRecursively();return;}install(result.helper);});
 watcher->setFuture(QtConcurrent::run([releaseCopy,source,directory,currentVersion]{Prepared result;auto zip=directory+"/download.zip";auto extracted=directory+"/new";QDir().mkpath(extracted);
  if(!verifyPackage(zip,releaseCopy,result.error)||!inspectArchive(zip,packageName(),result.error))return result;
#ifdef Q_OS_MACOS
  if(!runTool("/usr/bin/ditto",{"-x","-k",zip,extracted},result.error))return result;
#elif defined(Q_OS_WIN)
  auto tar=QDir::fromNativeSeparators(qEnvironmentVariable("SystemRoot"))+"/System32/tar.exe";if(!QFileInfo::exists(tar)||!runTool(tar,{"-xf",zip,"-C",extracted},result.error))return result;
#else
  if(!runTool(source+"/resources/tools/unzip",{"-q",zip,"-d",extracted},result.error))return result;
#endif
  if(!validateInstallation(extracted+"/"+packageName(),releaseCopy.version,result.error))return result;
  auto current=Version::parse(currentVersion);if(!current||!validateInstallation(source,*current,result.error))return result;
  // Run the helper from a complete private clone. This also keeps its Qt DLLs
  // outside the installation being replaced on Windows.
  auto runner=directory+"/runner/"+packageName();QDir().mkpath(QFileInfo(runner).absolutePath());if(!copyTree(source,runner,result.error))return result;result.helper=helperPath(runner);return result;
 }));
}
void Updater::install(const QString &helper){
 QString error;auto plan=QJsonObject{{"schema",1},{"target",root},{"staged",job+"/new/"+packageName()},{"version",release.version.text()},{"pid",QString::number(QCoreApplication::applicationPid())}};
 if(!writeJson(job+"/plan.json",plan,error)){manual=true;finish(error,true);return;}
 if(progress){progress->close();progress->deleteLater();progress=nullptr;}
 bool quit=QApplication::quitOnLastWindowClosed();QApplication::setQuitOnLastWindowClosed(false);
 if(!window->close()){QApplication::setQuitOnLastWindowClosed(quit);finish();QDir(job).removeRecursively();return;}
 if(reopenProject){auto project=reopenProject();if(QFileInfo(project).isFile()){plan["project"]=QFileInfo(project).absoluteFilePath();if(!writeJson(job+"/plan.json",plan,error)){window->show();QApplication::setQuitOnLastWindowClosed(quit);manual=true;finish(error,true);return;}}}
 if(!QProcess::startDetached(helper,{"--plan",job+"/plan.json"},QFileInfo(helper).absolutePath())){window->show();QApplication::setQuitOnLastWindowClosed(quit);manual=true;finish("Could not start the updater. The installed app is unchanged.",true);return;}
 QCoreApplication::quit();
}
}
