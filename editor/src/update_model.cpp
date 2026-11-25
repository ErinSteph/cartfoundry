#include "update_model.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QThread>
#include <QElapsedTimer>
#include <QUuid>
#include <filesystem>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#elif defined(Q_OS_MACOS)
#include <libproc.h>
#include <signal.h>
#else
#include <signal.h>
#include <unistd.h>
#endif
namespace cf::updates {
std::optional<Version> Version::parse(const QString &text){
 if(!QRegularExpression("^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$").match(text).hasMatch())return {};
 Version result;auto words=text.split('.');for(int i=0;i<3;++i){bool ok;auto n=words[i].toULongLong(&ok);if(!ok||n>2147483647)return {};result.parts[i]=quint32(n);}return result;
}
QString Version::text()const{return QString("%1.%2.%3").arg(parts[0]).arg(parts[1]).arg(parts[2]);}
QString platformKey(){
#if defined(Q_OS_MACOS) && defined(Q_PROCESSOR_ARM_64)
 return "macos-arm64";
#elif defined(Q_OS_MACOS) && defined(Q_PROCESSOR_X86_64)
 return "macos-x86_64";
#elif defined(Q_OS_WIN) && defined(Q_PROCESSOR_ARM_64)
 return "windows-arm64";
#elif defined(Q_OS_WIN) && defined(Q_PROCESSOR_X86_64)
 return "windows-x86_64";
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64)
 return "linux-x86_64";
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_ARM_64)
 return "linux-arm64";
#else
 return "unsupported";
#endif
}
QString packageName(){
#ifdef Q_OS_MACOS
 return "CartFoundry.app";
#else
 return "CartFoundry";
#endif
}
bool trustedPackageUrl(const QUrl &u){return u.isValid()&&u.scheme()=="https"&&u.host()=="github.com"&&u.userInfo().isEmpty()&&u.port(-1)==-1&&u.fragment().isEmpty()&&u.query().isEmpty()&&u.path().startsWith("/ErinSteph/cartfoundry/releases/download/")&&u.path().endsWith(".zip");}
bool parseManifest(const QByteArray &bytes,const QString &platform,Release &out,bool &available,QString &error){
 available=false;QJsonParseError parse;auto doc=QJsonDocument::fromJson(bytes,&parse);auto o=doc.object();auto version=Version::parse(o["version"].toString());
 if(bytes.size()>256*1024||parse.error!=QJsonParseError::NoError||!doc.isObject()||o["schema"]!=1||o["product"]!="CartFoundry"||!version||!o["platforms"].isObject()){error="Invalid CartFoundry update metadata.";return false;}
 out={};out.version=*version;out.notes=o["notes"].toString().left(8192);out.platform=platform;
 auto platforms=o["platforms"].toObject();if(!platforms.contains(platform))return true;
 auto p=platforms[platform].toObject();out.url=QUrl(p["url"].toString());auto hash=p["sha256"].toString();double size=p["size"].toDouble(-1);out.size=qint64(size);
 if(!trustedPackageUrl(out.url)||!QRegularExpression("^[a-fA-F0-9]{64}$").match(hash).hasMatch()||size<=0||size>1024.0*1024*1024||size!=out.size){error="Invalid platform download metadata.";return false;}
 out.sha256=QByteArray::fromHex(hash.toLatin1());available=true;return true;
}
void observeInstalledVersion(QSettings &s,const Version &current){auto previous=Version::parse(s.value("updates/installedVersion").toString());if(previous&&*previous<current)s.setValue("updates/automaticPrompts",true);s.setValue("updates/installedVersion",current.text());s.sync();}
bool verifyPackage(const QString &path,const Release &r,QString &error){QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()!=r.size){error="The update download is incomplete or has the wrong size.";return false;}QCryptographicHash hash(QCryptographicHash::Sha256);if(!hash.addData(&file)||hash.result()!=r.sha256){error="The update checksum does not match. The installed app has not been changed.";return false;}return true;}
QString metadataPath(const QString &root){
#ifdef Q_OS_MACOS
 return root+"/Contents/Resources/app-version.json";
#else
 return root+"/resources/app-version.json";
#endif
}
QString executablePath(const QString &root){
#ifdef Q_OS_MACOS
 return root+"/Contents/MacOS/CartFoundry";
#elif defined(Q_OS_WIN)
 return root+"/CartFoundry.exe";
#else
 return root+"/CartFoundry";
#endif
}
QJsonObject readJson(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>256*1024)return {};return QJsonDocument::fromJson(f.readAll()).object();}
bool writeJson(const QString &path,const QJsonObject &o,QString &error){QSaveFile f(path);auto bytes=QJsonDocument(o).toJson();if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit()){error=f.errorString();return false;}return true;}
bool runTool(const QString &exe,const QStringList &args,QString &error,int timeout){QProcess p;p.setProcessChannelMode(QProcess::MergedChannels);p.start(exe,args);if(!p.waitForStarted(10000)){error=p.errorString();return false;}if(!p.waitForFinished(timeout)){p.kill();p.waitForFinished();error="Update preparation timed out.";return false;}if(p.exitStatus()!=QProcess::NormalExit||p.exitCode()!=0){error=QString::fromLocal8Bit(p.readAll()).left(4096);return false;}return true;}
bool validateInstallation(const QString &root,const Version &version,QString &error,bool signature){
 auto o=readJson(metadataPath(root));if(QFileInfo(root).isSymLink()||o["product"]!="CartFoundry"||o["version"]!=version.text()||o["platform"]!=platformKey()||!QFileInfo(executablePath(root)).isFile()){error="The package identity, platform or version does not match the update.";return false;}
#ifdef Q_OS_MACOS
 const auto resources=root+"/Contents/Resources";const auto helper=root+"/Contents/MacOS/cf-update-helper";const auto compiler=resources+"/tools/gbdk/bin/lcc";
#elif defined(Q_OS_WIN)
 const auto resources=root+"/resources";const auto helper=root+"/cf-update-helper.exe";const auto compiler=resources+"/tools/gbdk/bin/lcc.exe";
#else
 const auto resources=root+"/resources";const auto helper=root+"/cf-update-helper";const auto compiler=resources+"/tools/gbdk/bin/lcc";
#endif
 for(const auto &required:{helper,compiler,resources+"/runtime/main.c",resources+"/THIRD_PARTY.md"})if(!QFileInfo(required).isFile()){error="The update is missing bundled tools, runtime or license files.";return false;}
#ifdef Q_OS_MACOS
 if(signature&&!runTool("/usr/bin/codesign",{"--verify","--deep","--strict",root},error))return false;
#else
 Q_UNUSED(signature);
#endif
 return true;
}
bool copyTree(const QString &from,const QString &to,QString &error){
 if(QFileInfo::exists(to)){error="Update staging destination already exists.";return false;}
#ifdef Q_OS_MACOS
 return runTool("/usr/bin/ditto",{from,to},error);
#else
 std::error_code ec;std::filesystem::copy(std::filesystem::u8path(from.toUtf8().constData()),std::filesystem::u8path(to.toUtf8().constData()),std::filesystem::copy_options::recursive|std::filesystem::copy_options::copy_symlinks,ec);if(ec){error=QString::fromStdString(ec.message());return false;}return true;
#endif
}
bool parsePlan(const QString &file,InstallPlan &p,QString &error){
 auto o=readJson(file);auto v=Version::parse(o["version"].toString());p.target=o["target"].toString();p.staged=o["staged"].toString();p.job=QFileInfo(file).absolutePath();p.parentPid=o["pid"].toString().toLongLong();
 if(o["schema"]!=1||!v||!QDir::isAbsolutePath(p.target)||!QDir::isAbsolutePath(p.staged)||QFileInfo(p.target).fileName()!=packageName()||QFileInfo(p.staged).fileName()!=packageName()||QFileInfo(p.staged).isSymLink()||!QDir(p.job).exists()||!QDir::cleanPath(p.staged).startsWith(QDir::cleanPath(p.job)+"/")||QFileInfo(p.target).absolutePath()==QDir::rootPath()){error="Invalid update installation plan.";return false;}p.version=*v;return true;
}
bool replaceInstallation(const InstallPlan &p,QString &backup,QString &error,bool signature){
 auto current=Version::parse(readJson(metadataPath(p.target))["version"].toString());if(!current||!(*current<p.version)){error="The installed version changed, or the update is not newer.";return false;}if(!validateInstallation(p.target,*current,error,signature)||!validateInstallation(p.staged,p.version,error,signature))return false;
 if(installationIsRunning(p.target)){error="Another copy of CartFoundry is still running. Close all copies before updating.";return false;}
 const auto parent=QFileInfo(p.target).absolutePath();auto incoming=parent+"/.CartFoundry-incoming-"+QUuid::createUuid().toString(QUuid::Id128);
#ifdef Q_OS_MACOS
 incoming+=".app";
#endif
 #ifdef Q_OS_MACOS
 backup=parent+"/.CartFoundry.previous-update.app";
#else
 backup=parent+"/.CartFoundry.previous-update";
#endif
 if(!copyTree(p.staged,incoming,error))return false;
 auto discardIncoming=[&]{QDir(incoming).removeRecursively();};
 if(!validateInstallation(incoming,p.version,error,signature)){discardIncoming();return false;}
 if(QFileInfo::exists(backup)){auto v=Version::parse(readJson(metadataPath(backup))["version"].toString());if(!v||*current<*v||!validateInstallation(backup,*v,error,signature)||!QDir(backup).removeRecursively()){error="Could not replace the previous update backup.";discardIncoming();return false;}}
 QDir dir;if(!dir.rename(p.target,backup)){error="Cannot replace this installation. Install in a folder you can write to, or update it manually.";discardIncoming();return false;}
 if(!dir.rename(incoming,p.target)){dir.rename(backup,p.target);discardIncoming();error="Could not install the update; the previous app was restored.";return false;}return true;
}
bool rollbackInstallation(const InstallPlan &p,const QString &backup,QString &error){
 if(installationIsRunning(p.target)){error="The new app is running but did not confirm startup. Its previous copy has been retained.";return false;}
 QDir dir;auto failed=p.target+".failed-update";if(QFileInfo::exists(failed)||!dir.rename(p.target,failed)){error="Could not move the failed update aside.";return false;}
 if(!dir.rename(backup,p.target)){dir.rename(failed,p.target);error="Automatic rollback failed; the previous app is at "+backup;return false;}QDir(failed).removeRecursively();return true;
}
bool waitForExit(qint64 pid,QString &error,int timeout){
 if(pid<=0){error="Invalid parent process.";return false;}
#ifdef Q_OS_WIN
 HANDLE process=OpenProcess(SYNCHRONIZE,FALSE,DWORD(pid));if(!process){if(GetLastError()==ERROR_INVALID_PARAMETER)return true;error="Cannot wait for CartFoundry to close.";return false;}auto result=WaitForSingleObject(process,DWORD(timeout));CloseHandle(process);if(result==WAIT_OBJECT_0)return true;
#else
 QElapsedTimer clock;clock.start();while(clock.elapsed()<timeout){if(kill(pid_t(pid),0)!=0&&errno==ESRCH)return true;QThread::msleep(100);}
#endif
 error="CartFoundry did not close. The update was not installed.";return false;
}
bool installationIsRunning(const QString &root){
 auto exe=QFileInfo(executablePath(root)).canonicalFilePath();if(exe.isEmpty())exe=QFileInfo(executablePath(root)).absoluteFilePath();
#ifdef Q_OS_WIN
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return true;PROCESSENTRY32W entry{};entry.dwSize=sizeof entry;bool running=false;
 if(Process32FirstW(snapshot,&entry))do{HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!p)continue;wchar_t path[32768];DWORD length=32768;if(QueryFullProcessImageNameW(p,0,path,&length)&&QString::fromWCharArray(path,int(length)).compare(QDir::toNativeSeparators(exe),Qt::CaseInsensitive)==0)running=true;CloseHandle(p);}while(!running&&Process32NextW(snapshot,&entry));CloseHandle(snapshot);return running;
#elif defined(Q_OS_MACOS)
 int size=proc_listallpids(nullptr,0);if(size<=0)return true;QVector<pid_t> pids(size+256);int count=proc_listallpids(pids.data(),int(pids.size()*sizeof(pid_t)));if(count<0)return true;
 for(int i=0;i<qMin(count,int(pids.size()));++i){char path[PROC_PIDPATHINFO_MAXSIZE];int n=proc_pidpath(pids[i],path,sizeof path);if(n>0&&QFileInfo(QString::fromUtf8(path)).canonicalFilePath()==exe)return true;}return false;
#elif defined(Q_OS_LINUX)
 QDir proc("/proc");if(!proc.exists())return true;for(const auto &entry:proc.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)){bool numeric;entry.fileName().toLongLong(&numeric);if(!numeric||entry.ownerId()!=getuid())continue;auto path=QFileInfo(entry.filePath()+"/exe").symLinkTarget();if(path.endsWith(" (deleted)"))path.chop(10);if(QDir::cleanPath(path)==exe)return true;
 // binfmt interpreters expose their own executable through /proc/pid/exe.
 // Check the guest executable mapping when running under Rosetta or QEMU.
 auto interpreter=QFileInfo(path).fileName();if(interpreter=="rosetta"||interpreter.startsWith("qemu-")){
  QFile maps(entry.filePath()+"/maps");if(!maps.open(QIODevice::ReadOnly))continue;
  auto escaped=QFile::encodeName(exe);escaped.replace("\\","\\134");escaped.replace(" ","\\040");escaped.replace("\t","\\011");escaped.replace("\n","\\012");
  for(const auto &line:maps.readAll().split('\n')){auto fields=line.simplified().split(' ');if(fields.size()>=6&&fields[1].contains('x')&&fields[5]==escaped)return true;}
 }
}return false;
#else
 Q_UNUSED(exe);return true;
#endif
}
}
