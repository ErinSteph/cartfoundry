#include "updater.h"
#include <QtTest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QAbstractButton>
#include <QMenu>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QTimer>
#include <cstring>
class Reply:public QNetworkReply {
 QByteArray data;qint64 position=0;
public:
 Reply(const QNetworkRequest &r,QNetworkAccessManager::Operation op,int status,QByteArray body,QObject *parent):QNetworkReply(parent),data(body){setRequest(r);setUrl(r.url());setOperation(op);setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);setHeader(QNetworkRequest::ContentLengthHeader,data.size());open(QIODevice::ReadOnly|QIODevice::Unbuffered);QTimer::singleShot(0,this,[this]{emit metaDataChanged();if(isFinished())return;if(!data.isEmpty())emit readyRead();if(isFinished())return;setFinished(true);emit finished();});}
 void abort()override{if(isFinished())return;setError(OperationCanceledError,"Cancelled");setFinished(true);emit finished();}
 qint64 bytesAvailable()const override{return data.size()-position+QNetworkReply::bytesAvailable();}
 qint64 readData(char *out,qint64 max)override{auto n=qMin(max,data.size()-position);if(!n)return -1;memcpy(out,data.constData()+position,size_t(n));position+=n;return n;}
};
class Network:public QNetworkAccessManager {
public:
 struct Request{Operation op;QNetworkRequest request;};QList<Request> requests;int head=200;QByteArray payload="bad update",feed;
 explicit Network(QObject *parent=nullptr):QNetworkAccessManager(parent){setVersion("0.9.1");}
 void setVersion(QString version){feed=QJsonDocument(QJsonObject{{"schema",1},{"product","CartFoundry"},{"version",version},{"platforms",QJsonObject{{cf::updates::platformKey(),QJsonObject{{"url","https://github.com/ErinSteph/cartfoundry/releases/download/v0.9.1/test.zip"},{"size",10},{"sha256",QString(64,'a')}}}}}}).toJson();}
 QNetworkReply *createRequest(Operation op,const QNetworkRequest &r,QIODevice *)override{requests.append({op,r});bool manifest=r.url()==QUrl(cf::updates::FeedUrl);return new Reply(r,op,manifest?200:op==HeadOperation?head:200,manifest?feed:op==HeadOperation?QByteArray():payload,this);}
};
class UpdateUiTests:public QObject {
 Q_OBJECT
 QString root;QTemporaryDir directory;
 void metadata(){root=directory.filePath(cf::updates::packageName());auto p=cf::updates::metadataPath(root);QDir().mkpath(QFileInfo(p).absolutePath());QFile f(p);QVERIFY(f.open(QIODevice::WriteOnly));f.write("fixture");
#ifdef Q_OS_MACOS
  auto helper=root+"/Contents/MacOS/cf-update-helper";
#elif defined(Q_OS_WIN)
  auto helper=root+"/cf-update-helper.exe";
#else
  auto helper=root+"/cf-update-helper";
#endif
  QDir().mkpath(QFileInfo(helper).absolutePath());QFile h(helper);QVERIFY(h.open(QIODevice::WriteOnly));h.write("fixture");
 }
 static void clickWhenOffered(QTimer &timer,QString button,bool &clicked){QObject::connect(&timer,&QTimer::timeout,&timer,[&timer,button,&clicked]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(!box)return;for(auto *b:box->buttons())if(b->text()==button){clicked=true;timer.stop();b->click();return;}});timer.start(5);}
private slots:
 void initTestCase(){QStandardPaths::setTestModeEnabled(true);QCoreApplication::setOrganizationName("CartFoundryUpdaterTests");QCoreApplication::setApplicationName("UpdaterUi");QCoreApplication::setApplicationVersion("0.9.0");metadata();}
 void init(){QSettings().clear();}
 void cleanupTestCase(){QSettings().clear();}
 void missingPackageNeverOffers(){QWidget window;Network network;network.head=404;cf::Updater updater(&window,&network,root);updater.check(false);QTRY_COMPARE(network.requests.size(),2);QTest::qWait(30);QVERIFY(!QApplication::activeModalWidget());QCOMPARE(network.requests[1].op,QNetworkAccessManager::HeadOperation);}
 void equalVersionDoesNotProbe(){QWidget window;Network network;network.setVersion("0.9.0");cf::Updater updater(&window,&network,root);updater.check(false);QTRY_COMPARE(network.requests.size(),1);QTest::qWait(30);QCOMPARE(network.requests.size(),1);QVERIFY(!QApplication::activeModalWidget());}
 void declineAndManualCheck(){QWidget window;QMenu menu;Network network;cf::Updater updater(&window,&network,root);updater.addToMenu(&menu);bool clicked=false;QTimer timer;clickWhenOffered(timer,"Don’t ask automatically",clicked);updater.check(false);QTRY_VERIFY(clicked);QVERIFY(!QSettings().value("updates/automaticPrompts",true).toBool());
  int count=network.requests.size();updater.start();QTest::qWait(1600);QCOMPARE(network.requests.size(),count);
  clicked=false;QTimer later;clickWhenOffered(later,"Later",clicked);updater.check(true);QTRY_VERIFY(clicked);QCOMPARE(network.requests.size(),count+2);QVERIFY(!QSettings().value("updates/automaticPrompts",true).toBool());
 }
 void laterOffersAgain(){QWidget window;Network network;cf::Updater updater(&window,&network,root);for(int i=0;i<2;++i){bool clicked=false;QTimer timer;clickWhenOffered(timer,"Later",clicked);updater.check(false);QTRY_VERIFY(clicked);}QCOMPARE(network.requests.size(),4);}
 void manualCheckDoesNotTriggerSecondStartupOffer(){QWidget window;Network network;cf::Updater updater(&window,&network,root);bool clicked=false;QTimer timer;clickWhenOffered(timer,"Later",clicked);updater.check(true);QTRY_VERIFY(clicked);int count=network.requests.size();updater.start();QTest::qWait(1600);QCOMPARE(network.requests.size(),count);}
 void headFallback(){QWidget window;Network network;network.head=405;cf::Updater updater(&window,&network,root);bool clicked=false;QTimer timer;clickWhenOffered(timer,"Later",clicked);updater.check(false);QTRY_VERIFY(clicked);QCOMPARE(network.requests.size(),3);QCOMPARE(network.requests.last().request.rawHeader("Range"),QByteArray("bytes=0-0"));}
 void corruptDownloadDoesNotCloseEditor(){QWidget window;window.show();Network network;cf::Updater updater(&window,&network,root);QTimer timer;bool offered=false,failed=false;
  connect(&timer,&QTimer::timeout,&timer,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(!box)return;if(box->text().contains("checksum")){failed=true;timer.stop();box->accept();return;}for(auto *b:box->buttons())if(b->text()=="Update"){offered=true;b->click();return;}});timer.start(5);updater.check(false);QTRY_VERIFY_WITH_TIMEOUT(failed,10000);QVERIFY(offered);QVERIFY(window.isVisible());QVERIFY(QFileInfo::exists(cf::updates::metadataPath(root)));
 }
};
QTEST_MAIN(UpdateUiTests)
#include "update_ui_tests.moc"
