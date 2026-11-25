#include "update_model.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QProcess>
using namespace cf::updates;
class UpdateTests:public QObject {
 Q_OBJECT
 static void put(const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(bytes),bytes.size());}
 static QJsonObject manifest(QString version="0.9.1"){return {{"schema",1},{"product","CartFoundry"},{"version",version},{"platforms",QJsonObject{{platformKey(),QJsonObject{{"url","https://github.com/ErinSteph/cartfoundry/releases/download/v0.9.1/CartFoundry.zip"},{"sha256",QString(64,'a')},{"size",100}}}}}};}
 static void fixture(const QString &root,QString version){
  auto meta=metadataPath(root);QString error;QDir().mkpath(QFileInfo(meta).absolutePath());QVERIFY(writeJson(meta,{{"product","CartFoundry"},{"version",version},{"platform",platformKey()}},error));
  auto exe=executablePath(root);QDir().mkpath(QFileInfo(exe).absolutePath());QVERIFY(QFile::copy(QString(CF_UPDATE_FIXTURE_PATH),exe));
#ifdef Q_OS_MACOS
  auto resources=root+"/Contents/Resources";QVERIFY(QFile::copy(QString(CF_UPDATE_HELPER_PATH),root+"/Contents/MacOS/cf-update-helper"));put(resources+"/tools/gbdk/bin/lcc","fixture");
  put(root+"/Contents/Info.plist",("<?xml version=\"1.0\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict><key>CFBundleIdentifier</key><string>cat.erin.cartfoundry.fixture</string><key>CFBundleExecutable</key><string>CartFoundry</string><key>CFBundlePackageType</key><string>APPL</string><key>CFBundleVersion</key><string>"+version+"</string></dict></plist>").toUtf8());
#elif defined(Q_OS_WIN)
  auto resources=root+"/resources";put(root+"/cf-update-helper.exe","fixture");put(resources+"/tools/gbdk/bin/lcc.exe","fixture");
#else
  auto resources=root+"/resources";put(root+"/cf-update-helper","fixture");put(resources+"/tools/gbdk/bin/lcc","fixture");
#endif
  put(resources+"/runtime/main.c","fixture");put(resources+"/THIRD_PARTY.md","fixture");
#ifdef Q_OS_MACOS
  QVERIFY2(runTool("/usr/bin/codesign",{"--force","--deep","--sign","-",root},error),qPrintable(error));
#endif
 }
private slots:
 void versions(){auto v=[](QString s){return Version::parse(s).value();};QVERIFY(v("0.7.2")<v("0.7.3"));QVERIFY(v("0.9.9")<v("0.10.0"));QVERIFY(v("0.99.99")<v("1.0.0"));QVERIFY(!(v("0.9.0")<v("0.9.0")));QVERIFY(!(v("1.0.0")<v("0.99.99")));for(QString s:{"0.9","0.9.0.1","v0.9.0","0.9.-1","0.09.0","2147483648.0.0","0.9.1-beta"})QVERIFY(!Version::parse(s));}
 void metadata(){Release release;bool available;QString error;auto o=manifest();QVERIFY(parseManifest(QJsonDocument(o).toJson(),platformKey(),release,available,error));QVERIFY(available);QCOMPARE(release.version.text(),QString("0.9.1"));QVERIFY(parseManifest(QJsonDocument(o).toJson(),"unpublished-platform",release,available,error));QVERIFY(!available);
  for(QString url:{"http://github.com/ErinSteph/cartfoundry/releases/download/v1/a.zip","https://evil.example/a.zip","https://github.com/Other/repo/releases/download/v1/a.zip","https://github.com/ErinSteph/cartfoundry/releases/download/v1/a.zip?token=x"})QVERIFY(!trustedPackageUrl(QUrl(url)));
  o["version"]="nonsense";QVERIFY(!parseManifest(QJsonDocument(o).toJson(),platformKey(),release,available,error));
 }
 void preferences(){QTemporaryDir dir;QSettings settings(dir.filePath("settings.ini"),QSettings::IniFormat);observeInstalledVersion(settings,Version::parse("0.9.0").value());settings.setValue("updates/automaticPrompts",false);observeInstalledVersion(settings,Version::parse("0.9.0").value());QVERIFY(!settings.value("updates/automaticPrompts").toBool());observeInstalledVersion(settings,Version::parse("0.9.1").value());QVERIFY(settings.value("updates/automaticPrompts").toBool());}
 void checksums(){QTemporaryDir dir;auto file=dir.filePath("download.zip");put(file,"wrong package");Release release;release.size=13;release.sha256=QByteArray(32,'a');QString error;QVERIFY(!verifyPackage(file,release,error));release.size=1;QVERIFY(!verifyPackage(file,release,error));}
 void archiveValidation(){
  QTemporaryDir dir;QString error;auto create=[&](QString name,QJsonObject entries){auto data=QJsonDocument(entries).toJson(QJsonDocument::Compact);auto path=dir.filePath(name+".zip");QProcess p;p.start("python3",{"-c","import json,sys,zipfile,stat; z=zipfile.ZipFile(sys.argv[1],'w'); entries=json.loads(sys.argv[2]);\nfor n,v in entries.items():\n i=zipfile.ZipInfo(n); i.create_system=3; i.external_attr=((stat.S_IFLNK|0o777) if v.startswith('LINK:') else (stat.S_IFREG|0o644))<<16; z.writestr(i,v[5:] if v.startswith('LINK:') else v)\nz.close()",path,QString::fromUtf8(data)});if(!p.waitForFinished()||p.exitCode()!=0)return QString();return path;};
  auto good=create("good",{{packageName()+"/Contents/file","value"}});QVERIFY(!good.isEmpty());QVERIFY2(inspectArchive(good,packageName(),error),qPrintable(error));
  for(auto pair:{qMakePair(QString("traversal"),QString("../outside")),qMakePair(QString("absolute"),QString("/tmp/outside")),qMakePair(QString("drive"),QString("C:/outside"))}){auto zip=create(pair.first,{{pair.second,"value"}});QVERIFY(!inspectArchive(zip,packageName(),error));}
  auto escape=create("symlink",{{packageName()+"/Contents/link","LINK:../../outside"}});QVERIFY(!inspectArchive(escape,packageName(),error));
  auto underneath=create("underlink",{{packageName()+"/Contents/link","LINK:file"},{packageName()+"/Contents/link/child","oops"},{packageName()+"/Contents/file","value"}});QVERIFY(!inspectArchive(underneath,packageName(),error));
 }
 void transaction(){QTemporaryDir dir;auto target=dir.filePath(packageName()),staged=dir.filePath("job/new/"+packageName());fixture(target,"0.9.0");fixture(staged,"0.9.1");InstallPlan p{target,staged,dir.filePath("job"),Version::parse("0.9.1").value(),0};QString error,backup;QVERIFY2(replaceInstallation(p,backup,error),qPrintable(error));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.1"));QCOMPARE(readJson(metadataPath(backup))["version"].toString(),QString("0.9.0"));QVERIFY2(rollbackInstallation(p,backup,error),qPrintable(error));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.0"));}
 void wrongPackageLeavesOldApp(){QTemporaryDir dir;auto target=dir.filePath(packageName()),staged=dir.filePath("job/new/"+packageName());fixture(target,"0.9.0");fixture(staged,"0.9.2");InstallPlan p{target,staged,dir.filePath("job"),Version::parse("0.9.1").value(),0};QString error,backup;QVERIFY(!replaceInstallation(p,backup,error));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.0"));}
 void runningAppIsNotReplaced(){
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN) || defined(Q_OS_LINUX)
  QTemporaryDir dir;auto target=dir.filePath(packageName()),staged=dir.filePath("job/new/"+packageName());fixture(target,"0.9.0");fixture(staged,"0.9.1");QProcess parent;parent.start(executablePath(target),{"--hold"});QVERIFY(parent.waitForStarted());InstallPlan p{target,staged,dir.filePath("job"),Version::parse("0.9.1").value(),0};QString error,backup;QVERIFY(!replaceInstallation(p,backup,error));QVERIFY2(error.contains("still running"),qPrintable(error));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.0"));QVERIFY(parent.waitForFinished(5000));
#else
  QSKIP("Running-process detection is verified on macOS; Windows remains unverified.");
#endif
 }
 void helperWaitsReplacesAndRelaunches(){
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN) || defined(Q_OS_LINUX)
  QTemporaryDir dir;auto target=dir.filePath(packageName()),staged=dir.filePath("job/new/"+packageName());fixture(target,"0.9.0");fixture(staged,"0.9.1");QProcess parent;parent.start(executablePath(target),{"--hold"});QVERIFY(parent.waitForStarted());
  auto job=dir.filePath("job");QString error;QVERIFY(writeJson(job+"/plan.json",{{"schema",1},{"target",target},{"staged",staged},{"version","0.9.1"},{"pid",QString::number(parent.processId())}},error));QProcess helper;helper.start(QString(CF_UPDATE_HELPER_PATH),{"--plan",job+"/plan.json"});QVERIFY(helper.waitForStarted());QTest::qWait(100);QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.0"));QVERIFY(parent.waitForFinished(5000));QVERIFY2(helper.waitForFinished(30000),qPrintable(QString::fromLocal8Bit(helper.readAllStandardError())));QCOMPARE(helper.exitCode(),0);QCOMPARE(readJson(job+"/result.json")["state"].toString(),QString("success"));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.1"));parent.waitForFinished();
#else
  QSKIP("Native end-to-end relaunch is currently verified on macOS only.");
#endif
 }
 void failedRelaunchRollsBack(){
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN) || defined(Q_OS_LINUX)
  QTemporaryDir dir;auto target=dir.filePath(packageName()),staged=dir.filePath("job/new/"+packageName());fixture(target,"0.9.0");fixture(staged,"0.9.1");put(QFileInfo(metadataPath(staged)).absolutePath()+"/fail-startup","1");QString error;
#ifdef Q_OS_MACOS
  QVERIFY(runTool("/usr/bin/codesign",{"--force","--deep","--sign","-",staged},error));
#endif
  QProcess parent;parent.start(executablePath(target),{"--hold"});QVERIFY(parent.waitForStarted());auto job=dir.filePath("job");QVERIFY(writeJson(job+"/plan.json",{{"schema",1},{"target",target},{"staged",staged},{"version","0.9.1"},{"pid",QString::number(parent.processId())}},error));QProcess helper;helper.start(QString(CF_UPDATE_HELPER_PATH),{"--plan",job+"/plan.json"});QVERIFY(helper.waitForStarted());QVERIFY(parent.waitForFinished(5000));QVERIFY(helper.waitForFinished(30000));QCOMPARE(helper.exitCode(),1);QCOMPARE(readJson(job+"/result.json")["state"].toString(),QString("rolled-back"));QTRY_COMPARE(readJson(job+"/confirmed.json")["version"].toString(),QString("0.9.1"));QCOMPARE(readJson(metadataPath(target))["version"].toString(),QString("0.9.0"));
#else
  QSKIP("Native rollback/relaunch verification is currently macOS only.");
#endif
 }

};
QTEST_GUILESS_MAIN(UpdateTests)
#include "update_tests.moc"
