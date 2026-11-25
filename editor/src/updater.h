#pragma once
#include "update_model.h"
#include <QObject>
#include <QPointer>
#include <functional>
class QWidget;class QMenu;class QAction;class QNetworkAccessManager;class QNetworkReply;class QProgressDialog;
namespace cf {
class Updater:public QObject {
public:
 explicit Updater(QWidget *,QNetworkAccessManager *network=nullptr,const QString &installationRoot={},std::function<QString()> reopenProject={});
 void addToMenu(QMenu *);
 void start(const QString &updatePlan={});
 void check(bool manual=true);
private:
 QWidget *window;QNetworkAccessManager *network;QAction *automaticAction=nullptr;
 QPointer<QProgressDialog> progress;bool busy=false,manual=false,checkedThisLaunch=false;
 updates::Release release;QString root,job;std::function<QString()> reopenProject;
 void finish(const QString &message={},bool failure=false);
 void probe(bool range=false);
 void offer();void download();void prepare();void install(const QString &helper);
};
}
